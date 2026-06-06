From Stdlib Require Import ZArith List Lia.
From compcert Require Import Integers.
Require Import VST.floyd.proofauto.
Require Import SSZFormal.clight.ssz_serialize.
Require Import SSZFormal.model.ssz_endian_model.
Require Import SSZFormal.model.ssz_serialize_model.

Import ListNotations.
Local Open Scope Z_scope.

#[export] Instance CompSpecs : compspecs. make_compspecs prog. Defined.
Definition Vprog : varspecs. mk_varspecs prog. Defined.

Definition t_ssz_u16_bytes : type := tarray tuchar 2.
Definition t_ssz_u32_bytes : type := tarray tuchar 4.
Definition t_ssz_u64_bytes : type := tarray tuchar 8.
Definition t_ssz_u128_bytes : type := tarray tuchar 16.
Definition t_ssz_u256_bytes : type := tarray tuchar 32.

Definition ssz_memcpy_spec : ident * funspec :=
  DECLARE _memcpy
  WITH n : Z, contents : list val, dst : val, src : val,
       sh_dst : share, sh_src : share
  PRE [ tptr tvoid, tptr tvoid, tulong ]
    PROP (0 <= n <= Ptrofs.max_unsigned;
          n <= uint64_max;
          Zlength contents = n;
          writable_share sh_dst;
          readable_share sh_src)
    PARAMS (dst; src; Vlong (Int64.repr n))
    GLOBALS ()
    SEP (data_at_ sh_dst (tarray tuchar n) dst;
         data_at sh_src (tarray tuchar n) contents src)
  POST [ tptr tvoid ]
    PROP ()
    RETURN (dst)
    SEP (data_at sh_dst (tarray tuchar n) contents dst;
         data_at sh_src (tarray tuchar n) contents src).

Definition ssz_memset_zero_spec : ident * funspec :=
  DECLARE _memset
  WITH n : Z, dst : val, sh_dst : share
  PRE [ tptr tvoid, tint, tulong ]
    PROP (0 <= n <= Ptrofs.max_unsigned;
          n <= uint64_max;
          writable_share sh_dst)
    PARAMS (dst; Vint Int.zero; Vlong (Int64.repr n))
    GLOBALS ()
    SEP (data_at_ sh_dst (tarray tuchar n) dst)
  POST [ tptr tvoid ]
    PROP ()
    RETURN (dst)
    SEP (data_at sh_dst (tarray tuchar n)
           (Zrepeat (Vubyte Byte.zero) n) dst).

Definition ssz_internal_bits_to_bytes_spec : ident * funspec :=
  DECLARE _ssz_internal_bits_to_bytes
  WITH bit_count : Z, out_bytes : val, sh_out : share
  PRE [ tulong, tptr tulong ]
    PROP (0 <= bit_count <= uint64_max;
          writable_share sh_out)
    PARAMS (Vlong (Int64.repr bit_count); out_bytes)
    GLOBALS ()
    SEP (data_at_ sh_out tulong out_bytes)
  POST [ tbool ]
    PROP ()
    RETURN (if ssz_bits_to_bytes_ok bit_count
            then Vint Int.one else Vint Int.zero)
    SEP (if ssz_bits_to_bytes_ok bit_count
         then data_at sh_out tulong
                (Vlong (Int64.repr (ssz_bits_to_bytes bit_count))) out_bytes
         else data_at_ sh_out tulong out_bytes).

Definition ssz_internal_u64_to_size_spec : ident * funspec :=
  DECLARE _ssz_internal_u64_to_size
  WITH value : Z, out : val, sh_out : share
  PRE [ tulong, tptr tulong ]
    PROP (0 <= value <= uint64_max;
          writable_share sh_out)
    PARAMS (Vlong (Int64.repr value); out)
    GLOBALS ()
    SEP (data_at_ sh_out tulong out)
  POST [ tbool ]
    PROP ()
    RETURN (Vint Int.one)
    SEP (data_at sh_out tulong (Vlong (Int64.repr value)) out).

Definition ssz_internal_add_overflow_size_spec : ident * funspec :=
  DECLARE _ssz_internal_add_overflow_size
  WITH a : Z, b : Z, out : val, sh_out : share
  PRE [ tulong, tulong, tptr tulong ]
    PROP (0 <= a <= uint64_max;
          0 <= b <= uint64_max;
          a + b <= uint64_max;
          writable_share sh_out)
    PARAMS (Vlong (Int64.repr a); Vlong (Int64.repr b); out)
    GLOBALS ()
    SEP (data_at_ sh_out tulong out)
  POST [ tbool ]
    PROP ()
    RETURN (Vint Int.zero)
    SEP (data_at sh_out tulong (Vlong (Int64.repr (a + b))) out).

Definition ssz_u64_le_data_at (sh : share) (contents : list val)
    (p : val) : mpred :=
  match contents with
  | [byte0; byte1; byte2; byte3; byte4; byte5; byte6; byte7] =>
      (field_at sh tuchar nil byte0 p *
      field_at sh tuchar nil byte1 (offset_val 1 p) *
      field_at sh tuchar nil byte2 (offset_val 2 p) *
      field_at sh tuchar nil byte3 (offset_val 3 p) *
      field_at sh tuchar nil byte4 (offset_val 4 p) *
      field_at sh tuchar nil byte5 (offset_val 5 p) *
      field_at sh tuchar nil byte6 (offset_val 6 p) *
      field_at sh tuchar nil byte7 (offset_val 7 p))%logic
  | _ => FF
  end.

Definition ssz_u64_le_data_at_ (sh : share) (p : val) : mpred :=
  (field_at_ sh tuchar nil p *
  field_at_ sh tuchar nil (offset_val 1 p) *
  field_at_ sh tuchar nil (offset_val 2 p) *
  field_at_ sh tuchar nil (offset_val 3 p) *
  field_at_ sh tuchar nil (offset_val 4 p) *
  field_at_ sh tuchar nil (offset_val 5 p) *
  field_at_ sh tuchar nil (offset_val 6 p) *
  field_at_ sh tuchar nil (offset_val 7 p))%logic.

Inductive prepare_output_case : Type :=
| Prepare_output_len_null (out : val)
| Prepare_output_measure_only (out_len : val) (sh_len : share)
| Prepare_output_with_out (out : val) (out_len : val) (sh_len : share).

Definition prepare_output_out (c : prepare_output_case) : val :=
  match c with
  | Prepare_output_len_null out => out
  | Prepare_output_measure_only _ _ => nullval
  | Prepare_output_with_out out _ _ => out
  end.

Definition prepare_output_out_len (c : prepare_output_case) : val :=
  match c with
  | Prepare_output_len_null _ => nullval
  | Prepare_output_measure_only out_len _ => out_len
  | Prepare_output_with_out _ out_len _ => out_len
  end.

Definition prepare_output_has_out_len (c : prepare_output_case) : bool :=
  match c with
  | Prepare_output_len_null _ => false
  | _ => true
  end.

Definition prepare_output_has_out (c : prepare_output_case) : bool :=
  match c with
  | Prepare_output_with_out _ _ _ => true
  | _ => false
  end.

Definition prepare_output_prop (c : prepare_output_case) : Prop :=
  match c with
  | Prepare_output_len_null out => is_pointer_or_null out
  | Prepare_output_measure_only _ sh_len => writable_share sh_len
  | Prepare_output_with_out out _ sh_len =>
      isptr out /\ writable_share sh_len
  end.

Definition prepare_output_pre (c : prepare_output_case) : mpred :=
  match c with
  | Prepare_output_len_null _ => emp
  | Prepare_output_measure_only out_len sh_len => data_at_ sh_len tulong out_len
  | Prepare_output_with_out out out_len sh_len =>
      (data_at_ sh_len tulong out_len * valid_pointer out)%logic
  end.

Definition prepare_output_result (c : prepare_output_case)
    (required out_cap : Z) : Z :=
  ssz_internal_prepare_output_result
    (prepare_output_has_out_len c) (prepare_output_has_out c)
    required out_cap.

Definition prepare_output_post (c : prepare_output_case)
    (required : Z) : mpred :=
  match c with
  | Prepare_output_len_null _ => emp
  | Prepare_output_measure_only out_len sh_len =>
      data_at sh_len tulong
        (Vlong (Int64.repr (ssz_internal_prepare_output_len required)))
        out_len
  | Prepare_output_with_out out out_len sh_len =>
      (data_at sh_len tulong
        (Vlong (Int64.repr (ssz_internal_prepare_output_len required)))
        out_len * valid_pointer out)%logic
  end.

Definition ssz_internal_write_u16_le_spec : ident * funspec :=
  DECLARE _ssz_internal_write_u16_le
  WITH out : val, sh : share, value : Z
  PRE [ tptr tuchar, tushort ]
    PROP (writable_share sh; 0 <= value <= uint16_max)
    PARAMS (out; Vint (Int.repr value))
    GLOBALS ()
    SEP (data_at_ sh t_ssz_u16_bytes out)
  POST [ tvoid ]
    PROP ()
    RETURN ()
    SEP (data_at sh t_ssz_u16_bytes
           (map Vubyte (ssz_u16_le_bytes value)) out).

Definition ssz_internal_write_u32_le_spec : ident * funspec :=
  DECLARE _ssz_internal_write_u32_le
  WITH out : val, sh : share, value : Z
  PRE [ tptr tuchar, tuint ]
    PROP (writable_share sh; 0 <= value <= uint32_max)
    PARAMS (out; Vint (Int.repr value))
    GLOBALS ()
    SEP (data_at_ sh t_ssz_u32_bytes out)
  POST [ tvoid ]
    PROP ()
    RETURN ()
    SEP (data_at sh t_ssz_u32_bytes
           (map Vubyte (ssz_u32_le_bytes value)) out).

Definition ssz_internal_write_u64_le_spec : ident * funspec :=
  DECLARE _ssz_internal_write_u64_le
  WITH out : val, sh : share, value : Z
  PRE [ tptr tuchar, tulong ]
    PROP (writable_share sh; 0 <= value <= uint64_max)
    PARAMS (out; Vlong (Int64.repr value))
    GLOBALS ()
    SEP (ssz_u64_le_data_at_ sh out)
  POST [ tvoid ]
    PROP ()
    RETURN ()
    SEP (field_at sh tuchar nil (Vubyte (ssz_u64_le_byte0 value)) out;
         field_at sh tuchar nil (Vubyte (ssz_u64_le_byte1 value))
           (offset_val 1 out);
         field_at sh tuchar nil (Vubyte (ssz_u64_le_byte2 value))
           (offset_val 2 out);
         field_at sh tuchar nil (Vubyte (ssz_u64_le_byte3 value))
           (offset_val 3 out);
         field_at sh tuchar nil (Vubyte (ssz_u64_le_byte4 value))
           (offset_val 4 out);
         field_at sh tuchar nil (Vubyte (ssz_u64_le_byte5 value))
           (offset_val 5 out);
         field_at sh tuchar nil (Vubyte (ssz_u64_le_byte6 value))
           (offset_val 6 out);
         field_at sh tuchar nil (Vubyte (ssz_u64_le_byte7 value))
           (offset_val 7 out)).

Definition ssz_internal_prepare_output_spec : ident * funspec :=
  DECLARE _ssz_internal_prepare_output
  WITH c : prepare_output_case, required : Z, out_cap : Z
  PRE [ tulong, tptr tuchar, tulong, tptr tulong ]
    PROP (0 <= required <= uint64_max;
          0 <= out_cap <= uint64_max;
          prepare_output_prop c)
    PARAMS (Vlong (Int64.repr required); prepare_output_out c;
            Vlong (Int64.repr out_cap); prepare_output_out_len c)
    GLOBALS ()
    SEP (prepare_output_pre c)
  POST [ tint ]
    PROP ()
    RETURN (Vint (Int.repr (prepare_output_result c required out_cap)))
    SEP (prepare_output_post c required).

Inductive serialize_uint8_case : Type :=
| Serialize_uint8_null
| Serialize_uint8_buffer (out : val) (sh : share).

Definition serialize_uint8_out (c : serialize_uint8_case) : val :=
  match c with
  | Serialize_uint8_null => nullval
  | Serialize_uint8_buffer out _ => out
  end.

Definition serialize_uint8_prop (c : serialize_uint8_case) : Prop :=
  match c with
  | Serialize_uint8_null => True
  | Serialize_uint8_buffer _ sh => writable_share sh
  end.

Definition serialize_uint8_pre (c : serialize_uint8_case) : mpred :=
  match c with
  | Serialize_uint8_null => emp
  | Serialize_uint8_buffer out sh => field_at_ sh tuchar nil out
  end.

Definition serialize_uint8_result (c : serialize_uint8_case) : Z :=
  match c with
  | Serialize_uint8_null => ssz_err_invalid_argument
  | Serialize_uint8_buffer _ _ => ssz_success
  end.

Definition serialize_uint8_post (c : serialize_uint8_case)
    (value : Z) : mpred :=
  match c with
  | Serialize_uint8_null => emp
  | Serialize_uint8_buffer out sh =>
      field_at sh tuchar nil
        (Vubyte (ssz_serialize_uint8_byte value)) out
  end.

Inductive serialize_boolean_case : Type :=
| Serialize_boolean_null
| Serialize_boolean_buffer (out : val) (sh : share).

Definition serialize_boolean_out (c : serialize_boolean_case) : val :=
  match c with
  | Serialize_boolean_null => nullval
  | Serialize_boolean_buffer out _ => out
  end.

Definition serialize_boolean_prop (c : serialize_boolean_case) : Prop :=
  match c with
  | Serialize_boolean_null => True
  | Serialize_boolean_buffer _ sh => writable_share sh
  end.

Definition serialize_boolean_pre (c : serialize_boolean_case) : mpred :=
  match c with
  | Serialize_boolean_null => emp
  | Serialize_boolean_buffer out sh => field_at_ sh tuchar nil out
  end.

Definition serialize_boolean_has_out (c : serialize_boolean_case) : bool :=
  match c with
  | Serialize_boolean_null => false
  | Serialize_boolean_buffer _ _ => true
  end.

Definition serialize_boolean_result (c : serialize_boolean_case)
    (value : Z) : Z :=
  ssz_serialize_boolean_result (serialize_boolean_has_out c) value.

Definition serialize_boolean_post (c : serialize_boolean_case)
    (value : Z) : mpred :=
  match c with
  | Serialize_boolean_null => emp
  | Serialize_boolean_buffer out sh =>
      if value <=? 1
      then field_at sh tuchar nil
             (Vubyte (ssz_serialize_boolean_byte value)) out
      else field_at_ sh tuchar nil out
  end.

Inductive serialize_uint16_case : Type :=
| Serialize_uint16_null
| Serialize_uint16_buffer (out : val) (sh : share).

Definition serialize_uint16_out (c : serialize_uint16_case) : val :=
  match c with
  | Serialize_uint16_null => nullval
  | Serialize_uint16_buffer out _ => out
  end.

Definition serialize_uint16_prop (c : serialize_uint16_case) : Prop :=
  match c with
  | Serialize_uint16_null => True
  | Serialize_uint16_buffer _ sh => writable_share sh
  end.

Definition serialize_uint16_pre (c : serialize_uint16_case) : mpred :=
  match c with
  | Serialize_uint16_null => emp
  | Serialize_uint16_buffer out sh => data_at_ sh t_ssz_u16_bytes out
  end.

Definition serialize_uint16_result (c : serialize_uint16_case) : Z :=
  match c with
  | Serialize_uint16_null => ssz_err_invalid_argument
  | Serialize_uint16_buffer _ _ => ssz_success
  end.

Definition serialize_uint16_post (c : serialize_uint16_case)
    (value : Z) : mpred :=
  match c with
  | Serialize_uint16_null => emp
  | Serialize_uint16_buffer out sh =>
      data_at sh t_ssz_u16_bytes
        (map Vubyte (ssz_serialize_uint16_bytes value)) out
  end.

Inductive serialize_uint32_case : Type :=
| Serialize_uint32_null
| Serialize_uint32_buffer (out : val) (sh : share).

Definition serialize_uint32_out (c : serialize_uint32_case) : val :=
  match c with
  | Serialize_uint32_null => nullval
  | Serialize_uint32_buffer out _ => out
  end.

Definition serialize_uint32_prop (c : serialize_uint32_case) : Prop :=
  match c with
  | Serialize_uint32_null => True
  | Serialize_uint32_buffer _ sh => writable_share sh
  end.

Definition serialize_uint32_pre (c : serialize_uint32_case) : mpred :=
  match c with
  | Serialize_uint32_null => emp
  | Serialize_uint32_buffer out sh => data_at_ sh t_ssz_u32_bytes out
  end.

Definition serialize_uint32_result (c : serialize_uint32_case) : Z :=
  match c with
  | Serialize_uint32_null => ssz_err_invalid_argument
  | Serialize_uint32_buffer _ _ => ssz_success
  end.

Definition serialize_uint32_post (c : serialize_uint32_case)
    (value : Z) : mpred :=
  match c with
  | Serialize_uint32_null => emp
  | Serialize_uint32_buffer out sh =>
      data_at sh t_ssz_u32_bytes
        (map Vubyte (ssz_serialize_uint32_bytes value)) out
  end.

Inductive serialize_uint64_case : Type :=
| Serialize_uint64_null
| Serialize_uint64_buffer (out : val) (sh : share).

Definition serialize_uint64_out (c : serialize_uint64_case) : val :=
  match c with
  | Serialize_uint64_null => nullval
  | Serialize_uint64_buffer out _ => out
  end.

Definition serialize_uint64_prop (c : serialize_uint64_case) : Prop :=
  match c with
  | Serialize_uint64_null => True
  | Serialize_uint64_buffer _ sh => writable_share sh
  end.

Definition serialize_uint64_pre (c : serialize_uint64_case) : mpred :=
  match c with
  | Serialize_uint64_null => emp
  | Serialize_uint64_buffer out sh =>
      ssz_u64_le_data_at_ sh out
  end.

Definition serialize_uint64_result (c : serialize_uint64_case) : Z :=
  match c with
  | Serialize_uint64_null => ssz_err_invalid_argument
  | Serialize_uint64_buffer _ _ => ssz_success
  end.

Definition serialize_uint64_post (c : serialize_uint64_case)
    (value : Z) : mpred :=
  match c with
  | Serialize_uint64_null => emp
  | Serialize_uint64_buffer out sh =>
      ssz_u64_le_data_at sh (map Vubyte (ssz_serialize_uint64_bytes value))
        out
  end.

Inductive serialize_uint128_case : Type :=
| Serialize_uint128_value_null (out : val)
| Serialize_uint128_out_null (value : val)
| Serialize_uint128_bad_len (value out : val)
| Serialize_uint128_buffer
    (value out : val) (sh_value sh_out : share) (bytes : list byte).

Definition serialize_uint128_value (c : serialize_uint128_case) : val :=
  match c with
  | Serialize_uint128_value_null _ => nullval
  | Serialize_uint128_out_null value => value
  | Serialize_uint128_bad_len value _ => value
  | Serialize_uint128_buffer value _ _ _ _ => value
  end.

Definition serialize_uint128_out (c : serialize_uint128_case) : val :=
  match c with
  | Serialize_uint128_value_null out => out
  | Serialize_uint128_out_null _ => nullval
  | Serialize_uint128_bad_len _ out => out
  | Serialize_uint128_buffer _ out _ _ _ => out
  end.

Definition serialize_uint128_has_value (c : serialize_uint128_case) : bool :=
  match c with
  | Serialize_uint128_value_null _ => false
  | _ => true
  end.

Definition serialize_uint128_has_out (c : serialize_uint128_case) : bool :=
  match c with
  | Serialize_uint128_out_null _ => false
  | _ => true
  end.

Definition serialize_uint128_prop (c : serialize_uint128_case) : Prop :=
  match c with
  | Serialize_uint128_value_null out => is_pointer_or_null out
  | Serialize_uint128_out_null value => isptr value
  | Serialize_uint128_bad_len value out => isptr value /\ isptr out
  | Serialize_uint128_buffer _ _ sh_value sh_out bytes =>
      readable_share sh_value /\ writable_share sh_out /\ Zlength bytes = 16
  end.

Definition serialize_uint128_pre (c : serialize_uint128_case) : mpred :=
  match c with
  | Serialize_uint128_value_null _ => emp
  | Serialize_uint128_out_null value => valid_pointer value
  | Serialize_uint128_bad_len value out =>
      (valid_pointer value * valid_pointer out)%logic
  | Serialize_uint128_buffer value out sh_value sh_out bytes =>
      (data_at sh_value t_ssz_u128_bytes (map Vubyte bytes) value *
       data_at_ sh_out t_ssz_u128_bytes out)%logic
  end.

Definition serialize_uint128_result (c : serialize_uint128_case)
    (value_len : Z) : Z :=
  ssz_serialize_fixed_bytes_result
    (serialize_uint128_has_value c) (serialize_uint128_has_out c)
    16 value_len.

Definition serialize_uint128_post (c : serialize_uint128_case) : mpred :=
  match c with
  | Serialize_uint128_value_null _ => emp
  | Serialize_uint128_out_null value => valid_pointer value
  | Serialize_uint128_bad_len value out =>
      (valid_pointer value * valid_pointer out)%logic
  | Serialize_uint128_buffer value out sh_value sh_out bytes =>
      (data_at sh_out t_ssz_u128_bytes
         (map Vubyte (ssz_serialize_uint128_bytes bytes)) out *
       data_at sh_value t_ssz_u128_bytes (map Vubyte bytes) value)%logic
  end.

Definition serialize_uint128_len_prop (c : serialize_uint128_case)
    (value_len : Z) : Prop :=
  match c with
  | Serialize_uint128_bad_len _ _ => value_len <> 16
  | Serialize_uint128_buffer _ _ _ _ _ => value_len = 16
  | _ => True
  end.

Inductive serialize_uint256_case : Type :=
| Serialize_uint256_value_null (out : val)
| Serialize_uint256_out_null (value : val)
| Serialize_uint256_bad_len (value out : val)
| Serialize_uint256_buffer
    (value out : val) (sh_value sh_out : share) (bytes : list byte).

Definition serialize_uint256_value (c : serialize_uint256_case) : val :=
  match c with
  | Serialize_uint256_value_null _ => nullval
  | Serialize_uint256_out_null value => value
  | Serialize_uint256_bad_len value _ => value
  | Serialize_uint256_buffer value _ _ _ _ => value
  end.

Definition serialize_uint256_out (c : serialize_uint256_case) : val :=
  match c with
  | Serialize_uint256_value_null out => out
  | Serialize_uint256_out_null _ => nullval
  | Serialize_uint256_bad_len _ out => out
  | Serialize_uint256_buffer _ out _ _ _ => out
  end.

Definition serialize_uint256_has_value (c : serialize_uint256_case) : bool :=
  match c with
  | Serialize_uint256_value_null _ => false
  | _ => true
  end.

Definition serialize_uint256_has_out (c : serialize_uint256_case) : bool :=
  match c with
  | Serialize_uint256_out_null _ => false
  | _ => true
  end.

Definition serialize_uint256_prop (c : serialize_uint256_case) : Prop :=
  match c with
  | Serialize_uint256_value_null out => is_pointer_or_null out
  | Serialize_uint256_out_null value => isptr value
  | Serialize_uint256_bad_len value out => isptr value /\ isptr out
  | Serialize_uint256_buffer _ _ sh_value sh_out bytes =>
      readable_share sh_value /\ writable_share sh_out /\ Zlength bytes = 32
  end.

Definition serialize_uint256_pre (c : serialize_uint256_case) : mpred :=
  match c with
  | Serialize_uint256_value_null _ => emp
  | Serialize_uint256_out_null value => valid_pointer value
  | Serialize_uint256_bad_len value out =>
      (valid_pointer value * valid_pointer out)%logic
  | Serialize_uint256_buffer value out sh_value sh_out bytes =>
      (data_at sh_value t_ssz_u256_bytes (map Vubyte bytes) value *
       data_at_ sh_out t_ssz_u256_bytes out)%logic
  end.

Definition serialize_uint256_result (c : serialize_uint256_case)
    (value_len : Z) : Z :=
  ssz_serialize_fixed_bytes_result
    (serialize_uint256_has_value c) (serialize_uint256_has_out c)
    32 value_len.

Definition serialize_uint256_post (c : serialize_uint256_case) : mpred :=
  match c with
  | Serialize_uint256_value_null _ => emp
  | Serialize_uint256_out_null value => valid_pointer value
  | Serialize_uint256_bad_len value out =>
      (valid_pointer value * valid_pointer out)%logic
  | Serialize_uint256_buffer value out sh_value sh_out bytes =>
      (data_at sh_out t_ssz_u256_bytes
         (map Vubyte (ssz_serialize_uint256_bytes bytes)) out *
       data_at sh_value t_ssz_u256_bytes (map Vubyte bytes) value)%logic
  end.

Definition serialize_uint256_len_prop (c : serialize_uint256_case)
    (value_len : Z) : Prop :=
  match c with
  | Serialize_uint256_bad_len _ _ => value_len <> 32
  | Serialize_uint256_buffer _ _ _ _ _ => value_len = 32
  | _ => True
  end.

Definition ssz_serialize_uint16_spec : ident * funspec :=
  DECLARE _ssz_serialize_uint16
  WITH c : serialize_uint16_case, value : Z
  PRE [ tushort, tptr tuchar ]
    PROP (0 <= value <= uint16_max; serialize_uint16_prop c)
    PARAMS (Vint (Int.repr value); serialize_uint16_out c)
    GLOBALS ()
    SEP (serialize_uint16_pre c)
  POST [ tint ]
    PROP ()
    RETURN (Vint (Int.repr (serialize_uint16_result c)))
    SEP (serialize_uint16_post c value).

Definition ssz_serialize_uint8_spec : ident * funspec :=
  DECLARE _ssz_serialize_uint8
  WITH c : serialize_uint8_case, value : Z
  PRE [ tuchar, tptr tuchar ]
    PROP (0 <= value <= Byte.max_unsigned; serialize_uint8_prop c)
    PARAMS (Vint (Int.repr value); serialize_uint8_out c)
    GLOBALS ()
    SEP (serialize_uint8_pre c)
  POST [ tint ]
    PROP ()
    RETURN (Vint (Int.repr (serialize_uint8_result c)))
    SEP (serialize_uint8_post c value).

Definition ssz_serialize_boolean_spec : ident * funspec :=
  DECLARE _ssz_serialize_boolean
  WITH c : serialize_boolean_case, value : Z
  PRE [ tuchar, tptr tuchar ]
    PROP (0 <= value <= Byte.max_unsigned; serialize_boolean_prop c)
    PARAMS (Vint (Int.repr value); serialize_boolean_out c)
    GLOBALS ()
    SEP (serialize_boolean_pre c)
  POST [ tint ]
    PROP ()
    RETURN (Vint (Int.repr (serialize_boolean_result c value)))
    SEP (serialize_boolean_post c value).

Definition ssz_serialize_uint32_spec : ident * funspec :=
  DECLARE _ssz_serialize_uint32
  WITH c : serialize_uint32_case, value : Z
  PRE [ tuint, tptr tuchar ]
    PROP (0 <= value <= uint32_max; serialize_uint32_prop c)
    PARAMS (Vint (Int.repr value); serialize_uint32_out c)
    GLOBALS ()
    SEP (serialize_uint32_pre c)
  POST [ tint ]
    PROP ()
    RETURN (Vint (Int.repr (serialize_uint32_result c)))
    SEP (serialize_uint32_post c value).

Definition ssz_serialize_uint64_spec : ident * funspec :=
  DECLARE _ssz_serialize_uint64
  WITH c : serialize_uint64_case, value : Z
  PRE [ tulong, tptr tuchar ]
    PROP (0 <= value <= uint64_max; serialize_uint64_prop c)
    PARAMS (Vlong (Int64.repr value); serialize_uint64_out c)
    GLOBALS ()
    SEP (serialize_uint64_pre c)
  POST [ tint ]
    PROP ()
    RETURN (Vint (Int.repr (serialize_uint64_result c)))
    SEP (serialize_uint64_post c value).

Definition ssz_serialize_uint128_spec : ident * funspec :=
  DECLARE _ssz_serialize_uint128
  WITH c : serialize_uint128_case, value_len : Z
  PRE [ tptr tuchar, tulong, tptr tuchar ]
    PROP (0 <= value_len <= uint64_max;
          serialize_uint128_prop c;
          serialize_uint128_len_prop c value_len)
    PARAMS (serialize_uint128_value c; Vlong (Int64.repr value_len);
            serialize_uint128_out c)
    GLOBALS ()
    SEP (serialize_uint128_pre c)
  POST [ tint ]
    PROP ()
    RETURN (Vint (Int.repr (serialize_uint128_result c value_len)))
    SEP (serialize_uint128_post c).

Definition ssz_serialize_uint256_spec : ident * funspec :=
  DECLARE _ssz_serialize_uint256
  WITH c : serialize_uint256_case, value_len : Z
  PRE [ tptr tuchar, tulong, tptr tuchar ]
    PROP (0 <= value_len <= uint64_max;
          serialize_uint256_prop c;
          serialize_uint256_len_prop c value_len)
    PARAMS (serialize_uint256_value c; Vlong (Int64.repr value_len);
            serialize_uint256_out c)
    GLOBALS ()
    SEP (serialize_uint256_pre c)
  POST [ tint ]
    PROP ()
    RETURN (Vint (Int.repr (serialize_uint256_result c value_len)))
    SEP (serialize_uint256_post c).

Inductive serialize_output_case : Type :=
| Serialize_output_len_null (out : val)
| Serialize_output_measure (out_len : val) (sh_len : share)
| Serialize_output_buffer_small
    (out out_len : val) (sh_len : share)
| Serialize_output_buffer
    (out out_len : val) (sh_len sh_out : share).

Definition serialize_output_out (c : serialize_output_case) : val :=
  match c with
  | Serialize_output_len_null out => out
  | Serialize_output_measure _ _ => nullval
  | Serialize_output_buffer_small out _ _ => out
  | Serialize_output_buffer out _ _ _ => out
  end.

Definition serialize_output_out_len (c : serialize_output_case) : val :=
  match c with
  | Serialize_output_len_null _ => nullval
  | Serialize_output_measure out_len _ => out_len
  | Serialize_output_buffer_small _ out_len _ => out_len
  | Serialize_output_buffer _ out_len _ _ => out_len
  end.

Definition serialize_output_has_out_len
    (c : serialize_output_case) : bool :=
  match c with
  | Serialize_output_len_null _ => false
  | _ => true
  end.

Definition serialize_output_has_out (c : serialize_output_case) : bool :=
  match c with
  | Serialize_output_buffer_small _ _ _ => true
  | Serialize_output_buffer _ _ _ _ => true
  | _ => false
  end.

Definition serialize_output_prop
    (c : serialize_output_case) (required out_cap : Z) : Prop :=
  match c with
  | Serialize_output_len_null out => is_pointer_or_null out
  | Serialize_output_measure _ sh_len => writable_share sh_len
  | Serialize_output_buffer_small out _ sh_len =>
      isptr out /\ writable_share sh_len /\ out_cap < required
  | Serialize_output_buffer out _ sh_len sh_out =>
      isptr out /\ writable_share sh_len /\ writable_share sh_out /\
      required <= out_cap
  end.

Definition serialize_output_pre
    (c : serialize_output_case) (required : Z) : mpred :=
  match c with
  | Serialize_output_len_null _ => emp
  | Serialize_output_measure out_len sh_len =>
      data_at_ sh_len tulong out_len
  | Serialize_output_buffer_small out out_len sh_len =>
      (data_at_ sh_len tulong out_len * valid_pointer out)%logic
  | Serialize_output_buffer out out_len sh_len sh_out =>
      (data_at_ sh_len tulong out_len *
       data_at_ sh_out (tarray tuchar required) out)%logic
  end.

Definition serialize_output_result
    (c : serialize_output_case) (required out_cap : Z) : Z :=
  ssz_internal_prepare_output_result
    (serialize_output_has_out_len c)
    (serialize_output_has_out c) required out_cap.

Definition serialize_output_post
    (c : serialize_output_case) (required : Z)
    (contents : list byte) : mpred :=
  match c with
  | Serialize_output_len_null _ => emp
  | Serialize_output_measure out_len sh_len =>
      data_at sh_len tulong (Vlong (Int64.repr required)) out_len
  | Serialize_output_buffer_small out out_len sh_len =>
      (data_at sh_len tulong (Vlong (Int64.repr required)) out_len *
       valid_pointer out)%logic
  | Serialize_output_buffer out out_len sh_len sh_out =>
      (data_at sh_len tulong (Vlong (Int64.repr required)) out_len *
       data_at sh_out (tarray tuchar required)
         (map Vubyte contents) out)%logic
  end.

Inductive serialize_bitvector_case : Type :=
| Serialize_bitvector_zero
    (bits_le out out_len : val) (bits_le_len out_cap : Z)
| Serialize_bitvector_overflow
    (bits_le out out_len : val)
    (bits_le_len bit_count out_cap : Z)
| Serialize_bitvector_bits_null
    (out out_len : val)
    (bits_le_len bit_count out_cap : Z)
| Serialize_bitvector_bits_short
    (bits_le out out_len : val)
    (bits_le_len bit_count out_cap : Z)
| Serialize_bitvector_padding_invalid
    (bits_le out out_len : val) (sh_bits : share)
    (bytes : list byte) (bits_le_len bit_count out_cap : Z)
| Serialize_bitvector_valid
    (bits_le : val) (sh_bits : share) (bytes : list byte)
    (bits_le_len bit_count out_cap : Z)
    (out_case : serialize_output_case).

Definition serialize_bitvector_bits_le (c : serialize_bitvector_case) : val :=
  match c with
  | Serialize_bitvector_zero bits_le _ _ _ _ => bits_le
  | Serialize_bitvector_overflow bits_le _ _ _ _ _ => bits_le
  | Serialize_bitvector_bits_null _ _ _ _ _ => nullval
  | Serialize_bitvector_bits_short bits_le _ _ _ _ _ => bits_le
  | Serialize_bitvector_padding_invalid bits_le _ _ _ _ _ _ _ => bits_le
  | Serialize_bitvector_valid bits_le _ _ _ _ _ _ => bits_le
  end.

Definition serialize_bitvector_bits_le_len
    (c : serialize_bitvector_case) : Z :=
  match c with
  | Serialize_bitvector_zero _ _ _ bits_le_len _ => bits_le_len
  | Serialize_bitvector_overflow _ _ _ bits_le_len _ _ => bits_le_len
  | Serialize_bitvector_bits_null _ _ bits_le_len _ _ => bits_le_len
  | Serialize_bitvector_bits_short _ _ _ bits_le_len _ _ => bits_le_len
  | Serialize_bitvector_padding_invalid _ _ _ _ _ bits_le_len _ _ =>
      bits_le_len
  | Serialize_bitvector_valid _ _ _ bits_le_len _ _ _ => bits_le_len
  end.

Definition serialize_bitvector_bit_count
    (c : serialize_bitvector_case) : Z :=
  match c with
  | Serialize_bitvector_zero _ _ _ _ _ => 0
  | Serialize_bitvector_overflow _ _ _ _ bit_count _ => bit_count
  | Serialize_bitvector_bits_null _ _ _ bit_count _ => bit_count
  | Serialize_bitvector_bits_short _ _ _ _ bit_count _ => bit_count
  | Serialize_bitvector_padding_invalid _ _ _ _ _ _ bit_count _ =>
      bit_count
  | Serialize_bitvector_valid _ _ _ _ bit_count _ _ => bit_count
  end.

Definition serialize_bitvector_out (c : serialize_bitvector_case) : val :=
  match c with
  | Serialize_bitvector_zero _ out _ _ _ => out
  | Serialize_bitvector_overflow _ out _ _ _ _ => out
  | Serialize_bitvector_bits_null out _ _ _ _ => out
  | Serialize_bitvector_bits_short _ out _ _ _ _ => out
  | Serialize_bitvector_padding_invalid _ out _ _ _ _ _ _ => out
  | Serialize_bitvector_valid _ _ _ _ _ _ out_case =>
      serialize_output_out out_case
  end.

Definition serialize_bitvector_out_cap (c : serialize_bitvector_case) : Z :=
  match c with
  | Serialize_bitvector_zero _ _ _ _ out_cap => out_cap
  | Serialize_bitvector_overflow _ _ _ _ _ out_cap => out_cap
  | Serialize_bitvector_bits_null _ _ _ _ out_cap => out_cap
  | Serialize_bitvector_bits_short _ _ _ _ _ out_cap => out_cap
  | Serialize_bitvector_padding_invalid _ _ _ _ _ _ _ out_cap => out_cap
  | Serialize_bitvector_valid _ _ _ _ _ out_cap _ => out_cap
  end.

Definition serialize_bitvector_out_len (c : serialize_bitvector_case) : val :=
  match c with
  | Serialize_bitvector_zero _ _ out_len _ _ => out_len
  | Serialize_bitvector_overflow _ _ out_len _ _ _ => out_len
  | Serialize_bitvector_bits_null _ out_len _ _ _ => out_len
  | Serialize_bitvector_bits_short _ _ out_len _ _ _ => out_len
  | Serialize_bitvector_padding_invalid _ _ out_len _ _ _ _ _ => out_len
  | Serialize_bitvector_valid _ _ _ _ _ _ out_case =>
      serialize_output_out_len out_case
  end.

Definition serialize_bitvector_bytes
    (c : serialize_bitvector_case) : list byte :=
  match c with
  | Serialize_bitvector_padding_invalid _ _ _ _ bytes _ _ _ => bytes
  | Serialize_bitvector_valid _ _ bytes _ _ _ _ => bytes
  | _ => []
  end.

Definition serialize_bitvector_prop (c : serialize_bitvector_case) : Prop :=
  match c with
  | Serialize_bitvector_zero bits_le out out_len bits_le_len out_cap =>
      0 <= bits_le_len <= uint64_max /\
      0 <= out_cap <= uint64_max /\
      is_pointer_or_null bits_le /\
      is_pointer_or_null out /\
      is_pointer_or_null out_len
  | Serialize_bitvector_overflow bits_le out out_len
      bits_le_len bit_count out_cap =>
      0 <= bits_le_len <= uint64_max /\
      0 <= bit_count <= uint64_max /\
      bit_count <> 0 /\
      ssz_bits_to_bytes_ok bit_count = false /\
      0 <= out_cap <= uint64_max /\
      is_pointer_or_null bits_le /\
      is_pointer_or_null out /\
      is_pointer_or_null out_len
  | Serialize_bitvector_bits_null out out_len
      bits_le_len bit_count out_cap =>
      0 <= bits_le_len <= uint64_max /\
      0 <= bit_count <= uint64_max /\
      bit_count <> 0 /\
      ssz_bits_to_bytes_ok bit_count = true /\
      0 <= out_cap <= uint64_max /\
      is_pointer_or_null out /\
      is_pointer_or_null out_len
  | Serialize_bitvector_bits_short bits_le out out_len
      bits_le_len bit_count out_cap =>
      0 <= bits_le_len <= uint64_max /\
      0 <= bit_count <= uint64_max /\
      bit_count <> 0 /\
      ssz_bits_to_bytes_ok bit_count = true /\
      bits_le_len < ssz_bits_to_bytes bit_count /\
      0 <= out_cap <= uint64_max /\
      isptr bits_le /\
      is_pointer_or_null out /\
      is_pointer_or_null out_len
  | Serialize_bitvector_padding_invalid bits_le out out_len sh_bits
      bytes bits_le_len bit_count out_cap =>
      0 <= bits_le_len <= uint64_max /\
      0 <= bit_count <= uint64_max /\
      bit_count <> 0 /\
      ssz_bits_to_bytes_ok bit_count = true /\
      ssz_bits_to_bytes bit_count <= bits_le_len /\
      bit_count mod 8 <> 0 /\
      ssz_padding_valid bit_count bytes = false /\
      Zlength bytes = ssz_bits_to_bytes bit_count /\
      readable_share sh_bits /\
      0 <= out_cap <= uint64_max /\
      is_pointer_or_null out /\
      is_pointer_or_null out_len
  | Serialize_bitvector_valid bits_le sh_bits bytes
      bits_le_len bit_count out_cap out_case =>
      0 <= bits_le_len <= uint64_max /\
      0 <= bit_count <= uint64_max /\
      bit_count <> 0 /\
      ssz_bits_to_bytes_ok bit_count = true /\
      ssz_bits_to_bytes bit_count <= bits_le_len /\
      ssz_padding_valid bit_count bytes = true /\
      Zlength bytes = ssz_bits_to_bytes bit_count /\
      readable_share sh_bits /\
      0 <= out_cap <= uint64_max /\
      serialize_output_prop out_case
        (ssz_bits_to_bytes bit_count) out_cap
  end.

Definition serialize_bitvector_pre (c : serialize_bitvector_case) : mpred :=
  match c with
  | Serialize_bitvector_padding_invalid bits_le _ _ sh_bits
      bytes _ bit_count _ =>
      data_at sh_bits (tarray tuchar (ssz_bits_to_bytes bit_count))
        (map Vubyte bytes) bits_le
  | Serialize_bitvector_valid bits_le sh_bits bytes
      _ bit_count _ out_case =>
      (data_at sh_bits (tarray tuchar (ssz_bits_to_bytes bit_count))
         (map Vubyte bytes) bits_le *
       serialize_output_pre out_case (ssz_bits_to_bytes bit_count))%logic
  | _ => emp
  end.

Definition serialize_bitvector_result (c : serialize_bitvector_case) : Z :=
  ssz_serialize_bitvector_result
    match c with
    | Serialize_bitvector_bits_null _ _ _ _ _ => false
    | _ => true
    end
    match c with
    | Serialize_bitvector_valid _ _ _ _ _ _ out_case =>
        serialize_output_has_out_len out_case
    | _ => true
    end
    match c with
    | Serialize_bitvector_valid _ _ _ _ _ _ out_case =>
        serialize_output_has_out out_case
    | _ => false
    end
    (serialize_bitvector_bits_le_len c)
    (serialize_bitvector_bit_count c)
    (serialize_bitvector_out_cap c)
    (serialize_bitvector_bytes c).

Definition serialize_bitvector_post (c : serialize_bitvector_case) : mpred :=
  match c with
  | Serialize_bitvector_padding_invalid bits_le _ _ sh_bits
      bytes _ bit_count _ =>
      data_at sh_bits (tarray tuchar (ssz_bits_to_bytes bit_count))
        (map Vubyte bytes) bits_le
  | Serialize_bitvector_valid bits_le sh_bits bytes
      _ bit_count _ out_case =>
      (data_at sh_bits (tarray tuchar (ssz_bits_to_bytes bit_count))
         (map Vubyte bytes) bits_le *
       serialize_output_post out_case (ssz_bits_to_bytes bit_count)
         (ssz_serialize_bitvector_bytes bytes))%logic
  | _ => emp
  end.

Definition ssz_serialize_bitvector_spec : ident * funspec :=
  DECLARE _ssz_serialize_bitvector
  WITH c : serialize_bitvector_case
  PRE [ tptr tuchar, tulong, tulong, tptr tuchar, tulong, tptr tulong ]
    PROP (serialize_bitvector_prop c)
    PARAMS (serialize_bitvector_bits_le c;
            Vlong (Int64.repr (serialize_bitvector_bits_le_len c));
            Vlong (Int64.repr (serialize_bitvector_bit_count c));
            serialize_bitvector_out c;
            Vlong (Int64.repr (serialize_bitvector_out_cap c));
            serialize_bitvector_out_len c)
    GLOBALS ()
    SEP (serialize_bitvector_pre c)
  POST [ tint ]
    PROP ()
    RETURN (Vint (Int.repr (serialize_bitvector_result c)))
    SEP (serialize_bitvector_post c).

Inductive serialize_bitlist_case : Type :=
| Serialize_bitlist_limit_exceeded
    (bits_le out out_len : val)
    (bits_le_len bit_len bit_limit out_cap : Z)
| Serialize_bitlist_overflow
    (bits_le out out_len : val)
    (bits_le_len bit_len bit_limit out_cap : Z)
| Serialize_bitlist_empty_valid
    (bits_le : val) (bits_le_len bit_limit out_cap : Z)
    (out_case : serialize_output_case)
| Serialize_bitlist_bits_null
    (out out_len : val)
    (bits_le_len bit_len bit_limit out_cap : Z)
| Serialize_bitlist_bits_short
    (bits_le out out_len : val)
    (bits_le_len bit_len bit_limit out_cap : Z)
| Serialize_bitlist_padding_invalid
    (bits_le out out_len : val) (sh_bits : share)
    (bytes : list byte)
    (bits_le_len bit_len bit_limit out_cap : Z)
| Serialize_bitlist_valid
    (bits_le : val) (sh_bits : share) (bytes : list byte)
    (bits_le_len bit_len bit_limit out_cap : Z)
    (out_case : serialize_output_case).

Definition serialize_bitlist_bits_le (c : serialize_bitlist_case) : val :=
  match c with
  | Serialize_bitlist_limit_exceeded bits_le _ _ _ _ _ _ => bits_le
  | Serialize_bitlist_overflow bits_le _ _ _ _ _ _ => bits_le
  | Serialize_bitlist_empty_valid bits_le _ _ _ _ => bits_le
  | Serialize_bitlist_bits_null _ _ _ _ _ _ => nullval
  | Serialize_bitlist_bits_short bits_le _ _ _ _ _ _ => bits_le
  | Serialize_bitlist_padding_invalid bits_le _ _ _ _ _ _ _ _ => bits_le
  | Serialize_bitlist_valid bits_le _ _ _ _ _ _ _ => bits_le
  end.

Definition serialize_bitlist_bits_le_len (c : serialize_bitlist_case) : Z :=
  match c with
  | Serialize_bitlist_limit_exceeded _ _ _ bits_le_len _ _ _ => bits_le_len
  | Serialize_bitlist_overflow _ _ _ bits_le_len _ _ _ => bits_le_len
  | Serialize_bitlist_empty_valid _ bits_le_len _ _ _ => bits_le_len
  | Serialize_bitlist_bits_null _ _ bits_le_len _ _ _ => bits_le_len
  | Serialize_bitlist_bits_short _ _ _ bits_le_len _ _ _ => bits_le_len
  | Serialize_bitlist_padding_invalid _ _ _ _ _ bits_le_len _ _ _ =>
      bits_le_len
  | Serialize_bitlist_valid _ _ _ bits_le_len _ _ _ _ => bits_le_len
  end.

Definition serialize_bitlist_bit_len (c : serialize_bitlist_case) : Z :=
  match c with
  | Serialize_bitlist_limit_exceeded _ _ _ _ bit_len _ _ => bit_len
  | Serialize_bitlist_overflow _ _ _ _ bit_len _ _ => bit_len
  | Serialize_bitlist_empty_valid _ _ _ _ _ => 0
  | Serialize_bitlist_bits_null _ _ _ bit_len _ _ => bit_len
  | Serialize_bitlist_bits_short _ _ _ _ bit_len _ _ => bit_len
  | Serialize_bitlist_padding_invalid _ _ _ _ _ _ bit_len _ _ => bit_len
  | Serialize_bitlist_valid _ _ _ _ bit_len _ _ _ => bit_len
  end.

Definition serialize_bitlist_bit_limit (c : serialize_bitlist_case) : Z :=
  match c with
  | Serialize_bitlist_limit_exceeded _ _ _ _ _ bit_limit _ => bit_limit
  | Serialize_bitlist_overflow _ _ _ _ _ bit_limit _ => bit_limit
  | Serialize_bitlist_empty_valid _ _ bit_limit _ _ => bit_limit
  | Serialize_bitlist_bits_null _ _ _ _ bit_limit _ => bit_limit
  | Serialize_bitlist_bits_short _ _ _ _ _ bit_limit _ => bit_limit
  | Serialize_bitlist_padding_invalid _ _ _ _ _ _ _ bit_limit _ =>
      bit_limit
  | Serialize_bitlist_valid _ _ _ _ _ bit_limit _ _ => bit_limit
  end.

Definition serialize_bitlist_out (c : serialize_bitlist_case) : val :=
  match c with
  | Serialize_bitlist_limit_exceeded _ out _ _ _ _ _ => out
  | Serialize_bitlist_overflow _ out _ _ _ _ _ => out
  | Serialize_bitlist_empty_valid _ _ _ _ out_case =>
      serialize_output_out out_case
  | Serialize_bitlist_bits_null out _ _ _ _ _ => out
  | Serialize_bitlist_bits_short _ out _ _ _ _ _ => out
  | Serialize_bitlist_padding_invalid _ out _ _ _ _ _ _ _ => out
  | Serialize_bitlist_valid _ _ _ _ _ _ _ out_case =>
      serialize_output_out out_case
  end.

Definition serialize_bitlist_out_cap (c : serialize_bitlist_case) : Z :=
  match c with
  | Serialize_bitlist_limit_exceeded _ _ _ _ _ _ out_cap => out_cap
  | Serialize_bitlist_overflow _ _ _ _ _ _ out_cap => out_cap
  | Serialize_bitlist_empty_valid _ _ _ out_cap _ => out_cap
  | Serialize_bitlist_bits_null _ _ _ _ _ out_cap => out_cap
  | Serialize_bitlist_bits_short _ _ _ _ _ _ out_cap => out_cap
  | Serialize_bitlist_padding_invalid _ _ _ _ _ _ _ _ out_cap => out_cap
  | Serialize_bitlist_valid _ _ _ _ _ _ out_cap _ => out_cap
  end.

Definition serialize_bitlist_out_len (c : serialize_bitlist_case) : val :=
  match c with
  | Serialize_bitlist_limit_exceeded _ _ out_len _ _ _ _ => out_len
  | Serialize_bitlist_overflow _ _ out_len _ _ _ _ => out_len
  | Serialize_bitlist_empty_valid _ _ _ _ out_case =>
      serialize_output_out_len out_case
  | Serialize_bitlist_bits_null _ out_len _ _ _ _ => out_len
  | Serialize_bitlist_bits_short _ _ out_len _ _ _ _ => out_len
  | Serialize_bitlist_padding_invalid _ _ out_len _ _ _ _ _ _ => out_len
  | Serialize_bitlist_valid _ _ _ _ _ _ _ out_case =>
      serialize_output_out_len out_case
  end.

Definition serialize_bitlist_bytes (c : serialize_bitlist_case) : list byte :=
  match c with
  | Serialize_bitlist_padding_invalid _ _ _ _ bytes _ _ _ _ => bytes
  | Serialize_bitlist_valid _ _ bytes _ _ _ _ _ => bytes
  | _ => []
  end.

Definition serialize_bitlist_prop (c : serialize_bitlist_case) : Prop :=
  match c with
  | Serialize_bitlist_limit_exceeded
      bits_le out out_len bits_le_len bit_len bit_limit out_cap =>
      0 <= bits_le_len <= uint64_max /\
      0 <= bit_len <= uint64_max /\
      0 <= bit_limit <= uint64_max /\
      bit_limit <> uint64_max /\
      bit_limit < bit_len /\
      0 <= out_cap <= uint64_max /\
      is_pointer_or_null bits_le /\
      is_pointer_or_null out /\
      is_pointer_or_null out_len
  | Serialize_bitlist_overflow bits_le out out_len
      bits_le_len bit_len bit_limit out_cap =>
      0 <= bits_le_len <= uint64_max /\
      0 <= bit_len <= uint64_max /\
      0 <= bit_limit <= uint64_max /\
      (bit_limit = uint64_max \/ bit_len <= bit_limit) /\
      ssz_bits_to_bytes_ok bit_len = false /\
      0 <= out_cap <= uint64_max /\
      is_pointer_or_null bits_le /\
      is_pointer_or_null out /\
      is_pointer_or_null out_len
  | Serialize_bitlist_empty_valid bits_le bits_le_len bit_limit
      out_cap out_case =>
      0 <= bits_le_len <= uint64_max /\
      0 <= bit_limit <= uint64_max /\
      0 <= out_cap <= uint64_max /\
      serialize_output_prop out_case (ssz_bitlist_required 0) out_cap /\
      is_pointer_or_null bits_le
  | Serialize_bitlist_bits_null out out_len
      bits_le_len bit_len bit_limit out_cap =>
      0 <= bits_le_len <= uint64_max /\
      0 <= bit_len <= uint64_max /\
      bit_len <> 0 /\
      0 <= bit_limit <= uint64_max /\
      (bit_limit = uint64_max \/ bit_len <= bit_limit) /\
      ssz_bits_to_bytes_ok bit_len = true /\
      0 <= out_cap <= uint64_max /\
      is_pointer_or_null out /\
      is_pointer_or_null out_len
  | Serialize_bitlist_bits_short bits_le out out_len
      bits_le_len bit_len bit_limit out_cap =>
      0 <= bits_le_len <= uint64_max /\
      0 <= bit_len <= uint64_max /\
      bit_len <> 0 /\
      0 <= bit_limit <= uint64_max /\
      (bit_limit = uint64_max \/ bit_len <= bit_limit) /\
      ssz_bits_to_bytes_ok bit_len = true /\
      bits_le_len < ssz_bitlist_data_bytes bit_len /\
      0 <= out_cap <= uint64_max /\
      isptr bits_le /\
      is_pointer_or_null out /\
      is_pointer_or_null out_len
  | Serialize_bitlist_padding_invalid bits_le out out_len sh_bits
      bytes bits_le_len bit_len bit_limit out_cap =>
      0 <= bits_le_len <= uint64_max /\
      0 <= bit_len <= uint64_max /\
      bit_len <> 0 /\
      0 <= bit_limit <= uint64_max /\
      (bit_limit = uint64_max \/ bit_len <= bit_limit) /\
      ssz_bits_to_bytes_ok bit_len = true /\
      ssz_bitlist_data_bytes bit_len <= bits_le_len /\
      bit_len mod 8 <> 0 /\
      ssz_padding_valid bit_len bytes = false /\
      Zlength bytes = ssz_bitlist_data_bytes bit_len /\
      readable_share sh_bits /\
      0 <= out_cap <= uint64_max /\
      is_pointer_or_null out /\
      is_pointer_or_null out_len
  | Serialize_bitlist_valid bits_le sh_bits bytes
      bits_le_len bit_len bit_limit out_cap out_case =>
      0 <= bits_le_len <= uint64_max /\
      0 <= bit_len <= uint64_max /\
      bit_len <> 0 /\
      0 <= bit_limit <= uint64_max /\
      (bit_limit = uint64_max \/ bit_len <= bit_limit) /\
      ssz_bits_to_bytes_ok bit_len = true /\
      ssz_bitlist_data_bytes bit_len <= bits_le_len /\
      ssz_padding_valid bit_len bytes = true /\
      Zlength bytes = ssz_bitlist_data_bytes bit_len /\
      readable_share sh_bits /\
      0 <= out_cap <= uint64_max /\
      serialize_output_prop out_case
        (ssz_bitlist_required bit_len) out_cap
  end.

Definition serialize_bitlist_pre (c : serialize_bitlist_case) : mpred :=
  match c with
  | Serialize_bitlist_empty_valid _ _ _ _ out_case =>
      serialize_output_pre out_case (ssz_bitlist_required 0)
  | Serialize_bitlist_padding_invalid bits_le _ _ sh_bits
      bytes _ bit_len _ _ =>
      data_at sh_bits (tarray tuchar (ssz_bitlist_data_bytes bit_len))
        (map Vubyte bytes) bits_le
  | Serialize_bitlist_valid bits_le sh_bits bytes
      _ bit_len _ _ out_case =>
      (data_at sh_bits (tarray tuchar (ssz_bitlist_data_bytes bit_len))
         (map Vubyte bytes) bits_le *
       serialize_output_pre out_case (ssz_bitlist_required bit_len))%logic
  | _ => emp
  end.

Definition serialize_bitlist_result (c : serialize_bitlist_case) : Z :=
  ssz_serialize_bitlist_result
    match c with
    | Serialize_bitlist_bits_null _ _ _ _ _ _ => false
    | _ => true
    end
    match c with
    | Serialize_bitlist_empty_valid _ _ _ _ out_case =>
        serialize_output_has_out_len out_case
    | Serialize_bitlist_valid _ _ _ _ _ _ _ out_case =>
        serialize_output_has_out_len out_case
    | _ => true
    end
    match c with
    | Serialize_bitlist_empty_valid _ _ _ _ out_case =>
        serialize_output_has_out out_case
    | Serialize_bitlist_valid _ _ _ _ _ _ _ out_case =>
        serialize_output_has_out out_case
    | _ => false
    end
    (serialize_bitlist_bits_le_len c)
    (serialize_bitlist_bit_len c)
    (serialize_bitlist_bit_limit c)
    (serialize_bitlist_out_cap c)
    (serialize_bitlist_bytes c).

Definition serialize_bitlist_post (c : serialize_bitlist_case) : mpred :=
  match c with
  | Serialize_bitlist_empty_valid _ _ _ _ out_case =>
      serialize_output_post out_case (ssz_bitlist_required 0)
        (ssz_serialize_bitlist_bytes 0 [])
  | Serialize_bitlist_padding_invalid bits_le _ _ sh_bits
      bytes _ bit_len _ _ =>
      data_at sh_bits (tarray tuchar (ssz_bitlist_data_bytes bit_len))
        (map Vubyte bytes) bits_le
  | Serialize_bitlist_valid bits_le sh_bits bytes
      _ bit_len _ _ out_case =>
      (data_at sh_bits (tarray tuchar (ssz_bitlist_data_bytes bit_len))
         (map Vubyte bytes) bits_le *
       serialize_output_post out_case (ssz_bitlist_required bit_len)
         (ssz_serialize_bitlist_bytes bit_len bytes))%logic
  | _ => emp
  end.

Definition ssz_serialize_bitlist_spec : ident * funspec :=
  DECLARE _ssz_serialize_bitlist
  WITH c : serialize_bitlist_case
  PRE [ tptr tuchar, tulong, tulong, tulong, tptr tuchar, tulong, tptr tulong ]
    PROP (serialize_bitlist_prop c)
    PARAMS (serialize_bitlist_bits_le c;
            Vlong (Int64.repr (serialize_bitlist_bits_le_len c));
            Vlong (Int64.repr (serialize_bitlist_bit_len c));
            Vlong (Int64.repr (serialize_bitlist_bit_limit c));
            serialize_bitlist_out c;
            Vlong (Int64.repr (serialize_bitlist_out_cap c));
            serialize_bitlist_out_len c)
    GLOBALS ()
    SEP (serialize_bitlist_pre c)
  POST [ tint ]
    PROP ()
    RETURN (Vint (Int.repr (serialize_bitlist_result c)))
    SEP (serialize_bitlist_post c).

Definition Gprog : funspecs :=
  ltac:(with_library prog
    [ssz_memcpy_spec;
     ssz_internal_prepare_output_spec;
     ssz_internal_write_u16_le_spec;
     ssz_internal_write_u32_le_spec;
     ssz_internal_write_u64_le_spec;
     ssz_serialize_uint8_spec;
     ssz_serialize_uint16_spec;
     ssz_serialize_uint32_spec;
     ssz_serialize_uint64_spec;
     ssz_serialize_uint128_spec;
     ssz_serialize_uint256_spec;
     ssz_serialize_boolean_spec]).
