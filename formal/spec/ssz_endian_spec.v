From Stdlib Require Import ZArith List Lia.
From compcert Require Import Integers.
Require Import VST.floyd.proofauto.
Require Import SSZFormal.clight.ssz_endian.
Require Import SSZFormal.model.ssz_endian_model.

Import ListNotations.
Local Open Scope Z_scope.

#[export] Instance CompSpecs : compspecs. make_compspecs prog. Defined.
Definition Vprog : varspecs. mk_varspecs prog. Defined.

Definition t_ssz_u16_bytes : type := tarray tuchar 2.
Definition t_ssz_u32_bytes : type := tarray tuchar 4.
Definition t_ssz_u64_bytes : type := tarray tuchar 8.

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

Definition ssz_internal_read_u16_le_spec : ident * funspec :=
  DECLARE _ssz_internal_read_u16_le
  WITH input : val, sh : share, byte0 : byte, byte1 : byte
  PRE [ tptr tuchar ]
    PROP (readable_share sh)
    PARAMS (input)
    GLOBALS ()
    SEP (data_at sh t_ssz_u16_bytes (map Vubyte [byte0; byte1]) input)
  POST [ tushort ]
    PROP ()
    RETURN (Vint (Int.repr (ssz_u16_le_decode [byte0; byte1])))
    SEP (data_at sh t_ssz_u16_bytes (map Vubyte [byte0; byte1]) input).

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

Definition ssz_internal_read_u32_le_spec : ident * funspec :=
  DECLARE _ssz_internal_read_u32_le
  WITH input : val, sh : share, byte0 : byte, byte1 : byte,
       byte2 : byte, byte3 : byte
  PRE [ tptr tuchar ]
    PROP (readable_share sh)
    PARAMS (input)
    GLOBALS ()
    SEP (data_at sh t_ssz_u32_bytes
           (map Vubyte [byte0; byte1; byte2; byte3]) input)
  POST [ tuint ]
    PROP ()
    RETURN (Vint (Int.repr
      (ssz_u32_le_decode [byte0; byte1; byte2; byte3])))
    SEP (data_at sh t_ssz_u32_bytes
           (map Vubyte [byte0; byte1; byte2; byte3]) input).

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

Definition ssz_internal_read_u64_le_spec : ident * funspec :=
  DECLARE _ssz_internal_read_u64_le
  WITH input : val, sh : share, byte0 : byte, byte1 : byte,
       byte2 : byte, byte3 : byte, byte4 : byte, byte5 : byte,
       byte6 : byte, byte7 : byte
  PRE [ tptr tuchar ]
    PROP (readable_share sh)
    PARAMS (input)
    GLOBALS ()
    SEP (field_at sh tuchar nil (Vubyte byte0) input;
         field_at sh tuchar nil (Vubyte byte1) (offset_val 1 input);
         field_at sh tuchar nil (Vubyte byte2) (offset_val 2 input);
         field_at sh tuchar nil (Vubyte byte3) (offset_val 3 input);
         field_at sh tuchar nil (Vubyte byte4) (offset_val 4 input);
         field_at sh tuchar nil (Vubyte byte5) (offset_val 5 input);
         field_at sh tuchar nil (Vubyte byte6) (offset_val 6 input);
         field_at sh tuchar nil (Vubyte byte7) (offset_val 7 input))
  POST [ tulong ]
    PROP ()
    RETURN (Vlong (Int64.repr
      (ssz_u64_le_decode
        [byte0; byte1; byte2; byte3; byte4; byte5; byte6; byte7])))
    SEP (field_at sh tuchar nil (Vubyte byte0) input;
         field_at sh tuchar nil (Vubyte byte1) (offset_val 1 input);
         field_at sh tuchar nil (Vubyte byte2) (offset_val 2 input);
         field_at sh tuchar nil (Vubyte byte3) (offset_val 3 input);
         field_at sh tuchar nil (Vubyte byte4) (offset_val 4 input);
         field_at sh tuchar nil (Vubyte byte5) (offset_val 5 input);
         field_at sh tuchar nil (Vubyte byte6) (offset_val 6 input);
         field_at sh tuchar nil (Vubyte byte7) (offset_val 7 input)).

Definition Gprog : funspecs :=
  ltac:(with_library prog
    [ssz_internal_write_u16_le_spec; ssz_internal_read_u16_le_spec;
     ssz_internal_write_u32_le_spec; ssz_internal_read_u32_le_spec;
     ssz_internal_write_u64_le_spec; ssz_internal_read_u64_le_spec]).
