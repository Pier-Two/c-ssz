From Coq Require Import String List ZArith.
From compcert Require Import Coqlib Integers Floats AST Ctypes Cop Clight Clightdefs.
Import Clightdefs.ClightNotations.
Local Open Scope Z_scope.
Local Open Scope string_scope.
Local Open Scope clight_scope.

Module Info.
  Definition version := "3.17".
  Definition build_number := "".
  Definition build_tag := "".
  Definition build_branch := "".
  Definition arch := "aarch64".
  Definition model := "default".
  Definition abi := "apple".
  Definition bitsize := 64.
  Definition big_endian := false.
  Definition source_file := "/Users/multi/Documents/GitHub/ssz/c-ssz/src/ssz_serialize.c".
  Definition normalized := true.
End Info.

Definition __395 : ident := $"_395".
Definition __435 : ident := $"_435".
Definition __449 : ident := $"_449".
Definition ___builtin_annot : ident := $"__builtin_annot".
Definition ___builtin_annot_intval : ident := $"__builtin_annot_intval".
Definition ___builtin_bswap : ident := $"__builtin_bswap".
Definition ___builtin_bswap16 : ident := $"__builtin_bswap16".
Definition ___builtin_bswap32 : ident := $"__builtin_bswap32".
Definition ___builtin_bswap64 : ident := $"__builtin_bswap64".
Definition ___builtin_cls : ident := $"__builtin_cls".
Definition ___builtin_clsl : ident := $"__builtin_clsl".
Definition ___builtin_clsll : ident := $"__builtin_clsll".
Definition ___builtin_clz : ident := $"__builtin_clz".
Definition ___builtin_clzl : ident := $"__builtin_clzl".
Definition ___builtin_clzll : ident := $"__builtin_clzll".
Definition ___builtin_ctz : ident := $"__builtin_ctz".
Definition ___builtin_ctzl : ident := $"__builtin_ctzl".
Definition ___builtin_ctzll : ident := $"__builtin_ctzll".
Definition ___builtin_debug : ident := $"__builtin_debug".
Definition ___builtin_expect : ident := $"__builtin_expect".
Definition ___builtin_fabs : ident := $"__builtin_fabs".
Definition ___builtin_fabsf : ident := $"__builtin_fabsf".
Definition ___builtin_fmadd : ident := $"__builtin_fmadd".
Definition ___builtin_fmax : ident := $"__builtin_fmax".
Definition ___builtin_fmin : ident := $"__builtin_fmin".
Definition ___builtin_fmsub : ident := $"__builtin_fmsub".
Definition ___builtin_fnmadd : ident := $"__builtin_fnmadd".
Definition ___builtin_fnmsub : ident := $"__builtin_fnmsub".
Definition ___builtin_fsqrt : ident := $"__builtin_fsqrt".
Definition ___builtin_membar : ident := $"__builtin_membar".
Definition ___builtin_memcpy_aligned : ident := $"__builtin_memcpy_aligned".
Definition ___builtin_sel : ident := $"__builtin_sel".
Definition ___builtin_sqrt : ident := $"__builtin_sqrt".
Definition ___builtin_unreachable : ident := $"__builtin_unreachable".
Definition ___builtin_va_arg : ident := $"__builtin_va_arg".
Definition ___builtin_va_copy : ident := $"__builtin_va_copy".
Definition ___builtin_va_end : ident := $"__builtin_va_end".
Definition ___builtin_va_start : ident := $"__builtin_va_start".
Definition ___compcert_i64_dtos : ident := $"__compcert_i64_dtos".
Definition ___compcert_i64_dtou : ident := $"__compcert_i64_dtou".
Definition ___compcert_i64_sar : ident := $"__compcert_i64_sar".
Definition ___compcert_i64_sdiv : ident := $"__compcert_i64_sdiv".
Definition ___compcert_i64_shl : ident := $"__compcert_i64_shl".
Definition ___compcert_i64_shr : ident := $"__compcert_i64_shr".
Definition ___compcert_i64_smod : ident := $"__compcert_i64_smod".
Definition ___compcert_i64_smulh : ident := $"__compcert_i64_smulh".
Definition ___compcert_i64_stod : ident := $"__compcert_i64_stod".
Definition ___compcert_i64_stof : ident := $"__compcert_i64_stof".
Definition ___compcert_i64_udiv : ident := $"__compcert_i64_udiv".
Definition ___compcert_i64_umod : ident := $"__compcert_i64_umod".
Definition ___compcert_i64_umulh : ident := $"__compcert_i64_umulh".
Definition ___compcert_i64_utod : ident := $"__compcert_i64_utod".
Definition ___compcert_i64_utof : ident := $"__compcert_i64_utof".
Definition ___compcert_va_composite : ident := $"__compcert_va_composite".
Definition ___compcert_va_float64 : ident := $"__compcert_va_float64".
Definition ___compcert_va_int32 : ident := $"__compcert_va_int32".
Definition ___compcert_va_int64 : ident := $"__compcert_va_int64".
Definition _a : ident := $"a".
Definition _allowed : ident := $"allowed".
Definition _allowed_selector_count : ident := $"allowed_selector_count".
Definition _allowed_selectors : ident := $"allowed_selectors".
Definition _b : ident := $"b".
Definition _bit_count : ident := $"bit_count".
Definition _bit_len : ident := $"bit_len".
Definition _bit_limit : ident := $"bit_limit".
Definition _bits_le : ident := $"bits_le".
Definition _bits_le_len : ident := $"bits_le_len".
Definition _byte_count : ident := $"byte_count".
Definition _bytes : ident := $"bytes".
Definition _bytes_u64 : ident := $"bytes_u64".
Definition _codec : ident := $"codec".
Definition _contribution : ident := $"contribution".
Definition _converted : ident := $"converted".
Definition _ctx : ident := $"ctx".
Definition _cursor : ident := $"cursor".
Definition _data_bytes : ident := $"data_bytes".
Definition _delimiter_bit : ident := $"delimiter_bit".
Definition _delimiter_byte : ident := $"delimiter_byte".
Definition _element_count : ident := $"element_count".
Definition _element_limit : ident := $"element_limit".
Definition _element_size : ident := $"element_size".
Definition _elements : ident := $"elements".
Definition _encoded_len : ident := $"encoded_len".
Definition _err : ident := $"err".
Definition _expected_len : ident := $"expected_len".
Definition _field_count : ident := $"field_count".
Definition _field_fixed_sizes : ident := $"field_fixed_sizes".
Definition _fixed_cursor : ident := $"fixed_cursor".
Definition _fixed_region : ident := $"fixed_region".
Definition _fixed_size : ident := $"fixed_size".
Definition _fixed_size__1 : ident := $"fixed_size__1".
Definition _has_none : ident := $"has_none".
Definition _i : ident := $"i".
Definition _i__1 : ident := $"i__1".
Definition _i__2 : ident := $"i__2".
Definition _main : ident := $"main".
Definition _mask : ident := $"mask".
Definition _memcpy : ident := $"memcpy".
Definition _memset : ident := $"memset".
Definition _next_fixed_cursor : ident := $"next_fixed_cursor".
Definition _next_fixed_cursor__1 : ident := $"next_fixed_cursor__1".
Definition _option_count : ident := $"option_count".
Definition _out : ident := $"out".
Definition _out_bytes : ident := $"out_bytes".
Definition _out_cap : ident := $"out_cap".
Definition _out_len : ident := $"out_len".
Definition _overflow : ident := $"overflow".
Definition _payload_len : ident := $"payload_len".
Definition _read : ident := $"read".
Definition _required : ident := $"required".
Definition _root : ident := $"root".
Definition _schema : ident := $"schema".
Definition _selector : ident := $"selector".
Definition _ssz_internal_add_overflow_size : ident := $"ssz_internal_add_overflow_size".
Definition _ssz_internal_bits_to_bytes : ident := $"ssz_internal_bits_to_bytes".
Definition _ssz_internal_mul_overflow_size : ident := $"ssz_internal_mul_overflow_size".
Definition _ssz_internal_prepare_output : ident := $"ssz_internal_prepare_output".
Definition _ssz_internal_selector_allowed : ident := $"ssz_internal_selector_allowed".
Definition _ssz_internal_u64_to_size : ident := $"ssz_internal_u64_to_size".
Definition _ssz_internal_validate_compatible_union_schema : ident := $"ssz_internal_validate_compatible_union_schema".
Definition _ssz_internal_write_u16_le : ident := $"ssz_internal_write_u16_le".
Definition _ssz_internal_write_u32_le : ident := $"ssz_internal_write_u32_le".
Definition _ssz_internal_write_u64_le : ident := $"ssz_internal_write_u64_le".
Definition _ssz_serialize_bitlist : ident := $"ssz_serialize_bitlist".
Definition _ssz_serialize_bitvector : ident := $"ssz_serialize_bitvector".
Definition _ssz_serialize_boolean : ident := $"ssz_serialize_boolean".
Definition _ssz_serialize_compatible_union : ident := $"ssz_serialize_compatible_union".
Definition _ssz_serialize_container : ident := $"ssz_serialize_container".
Definition _ssz_serialize_list_fixed : ident := $"ssz_serialize_list_fixed".
Definition _ssz_serialize_list_variable : ident := $"ssz_serialize_list_variable".
Definition _ssz_serialize_uint128 : ident := $"ssz_serialize_uint128".
Definition _ssz_serialize_uint16 : ident := $"ssz_serialize_uint16".
Definition _ssz_serialize_uint256 : ident := $"ssz_serialize_uint256".
Definition _ssz_serialize_uint32 : ident := $"ssz_serialize_uint32".
Definition _ssz_serialize_uint64 : ident := $"ssz_serialize_uint64".
Definition _ssz_serialize_uint8 : ident := $"ssz_serialize_uint8".
Definition _ssz_serialize_union : ident := $"ssz_serialize_union".
Definition _ssz_serialize_vector_fixed : ident := $"ssz_serialize_vector_fixed".
Definition _ssz_serialize_vector_variable : ident := $"ssz_serialize_vector_variable".
Definition _total : ident := $"total".
Definition _value : ident := $"value".
Definition _value_len : ident := $"value_len".
Definition _variable_cursor : ident := $"variable_cursor".
Definition _write : ident := $"write".
Definition _written : ident := $"written".
Definition _t'1 : ident := 128%positive.
Definition _t'10 : ident := 137%positive.
Definition _t'11 : ident := 138%positive.
Definition _t'12 : ident := 139%positive.
Definition _t'13 : ident := 140%positive.
Definition _t'14 : ident := 141%positive.
Definition _t'15 : ident := 142%positive.
Definition _t'16 : ident := 143%positive.
Definition _t'17 : ident := 144%positive.
Definition _t'18 : ident := 145%positive.
Definition _t'19 : ident := 146%positive.
Definition _t'2 : ident := 129%positive.
Definition _t'20 : ident := 147%positive.
Definition _t'21 : ident := 148%positive.
Definition _t'22 : ident := 149%positive.
Definition _t'23 : ident := 150%positive.
Definition _t'24 : ident := 151%positive.
Definition _t'25 : ident := 152%positive.
Definition _t'26 : ident := 153%positive.
Definition _t'27 : ident := 154%positive.
Definition _t'28 : ident := 155%positive.
Definition _t'29 : ident := 156%positive.
Definition _t'3 : ident := 130%positive.
Definition _t'30 : ident := 157%positive.
Definition _t'31 : ident := 158%positive.
Definition _t'32 : ident := 159%positive.
Definition _t'33 : ident := 160%positive.
Definition _t'34 : ident := 161%positive.
Definition _t'35 : ident := 162%positive.
Definition _t'36 : ident := 163%positive.
Definition _t'37 : ident := 164%positive.
Definition _t'38 : ident := 165%positive.
Definition _t'39 : ident := 166%positive.
Definition _t'4 : ident := 131%positive.
Definition _t'40 : ident := 167%positive.
Definition _t'41 : ident := 168%positive.
Definition _t'42 : ident := 169%positive.
Definition _t'43 : ident := 170%positive.
Definition _t'44 : ident := 171%positive.
Definition _t'45 : ident := 172%positive.
Definition _t'46 : ident := 173%positive.
Definition _t'47 : ident := 174%positive.
Definition _t'48 : ident := 175%positive.
Definition _t'49 : ident := 176%positive.
Definition _t'5 : ident := 132%positive.
Definition _t'50 : ident := 177%positive.
Definition _t'51 : ident := 178%positive.
Definition _t'52 : ident := 179%positive.
Definition _t'53 : ident := 180%positive.
Definition _t'54 : ident := 181%positive.
Definition _t'55 : ident := 182%positive.
Definition _t'56 : ident := 183%positive.
Definition _t'57 : ident := 184%positive.
Definition _t'58 : ident := 185%positive.
Definition _t'59 : ident := 186%positive.
Definition _t'6 : ident := 133%positive.
Definition _t'60 : ident := 187%positive.
Definition _t'61 : ident := 188%positive.
Definition _t'62 : ident := 189%positive.
Definition _t'7 : ident := 134%positive.
Definition _t'8 : ident := 135%positive.
Definition _t'9 : ident := 136%positive.

Definition f_ssz_internal_add_overflow_size := {|
  fn_return := tbool;
  fn_callconv := cc_default;
  fn_params := ((_a, tulong) :: (_b, tulong) :: (_out, (tptr tulong)) :: nil);
  fn_vars := nil;
  fn_temps := ((_overflow, tbool) :: nil);
  fn_body :=
(Ssequence
  (Sset _overflow (Ecast (Econst_int (Int.repr 0) tint) tbool))
  (Ssequence
    (Sifthenelse (Ebinop Ogt (Etempvar _a tulong)
                   (Ebinop Osub (Econst_long (Int64.repr (-1)) tulong)
                     (Etempvar _b tulong) tulong) tint)
      (Sset _overflow (Ecast (Econst_int (Int.repr 1) tint) tbool))
      (Sifthenelse (Ebinop One (Etempvar _out (tptr tulong))
                     (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid))
                     tint)
        (Sassign (Ederef (Etempvar _out (tptr tulong)) tulong)
          (Ebinop Oadd (Etempvar _a tulong) (Etempvar _b tulong) tulong))
        Sskip))
    (Sreturn (Some (Etempvar _overflow tbool)))))
|}.

Definition f_ssz_internal_mul_overflow_size := {|
  fn_return := tbool;
  fn_callconv := cc_default;
  fn_params := ((_a, tulong) :: (_b, tulong) :: (_out, (tptr tulong)) :: nil);
  fn_vars := nil;
  fn_temps := ((_overflow, tbool) :: (_t'1, tint) :: nil);
  fn_body :=
(Ssequence
  (Sset _overflow (Ecast (Econst_int (Int.repr 0) tint) tbool))
  (Ssequence
    (Ssequence
      (Sifthenelse (Ebinop One (Etempvar _a tulong)
                     (Econst_int (Int.repr 0) tuint) tint)
        (Sset _t'1
          (Ecast
            (Ebinop Ogt (Etempvar _b tulong)
              (Ebinop Odiv (Econst_long (Int64.repr (-1)) tulong)
                (Etempvar _a tulong) tulong) tint) tbool))
        (Sset _t'1 (Econst_int (Int.repr 0) tint)))
      (Sifthenelse (Etempvar _t'1 tint)
        (Sset _overflow (Ecast (Econst_int (Int.repr 1) tint) tbool))
        (Sifthenelse (Ebinop One (Etempvar _out (tptr tulong))
                       (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid))
                       tint)
          (Sassign (Ederef (Etempvar _out (tptr tulong)) tulong)
            (Ebinop Omul (Etempvar _a tulong) (Etempvar _b tulong) tulong))
          Sskip)))
    (Sreturn (Some (Etempvar _overflow tbool)))))
|}.

Definition f_ssz_internal_u64_to_size := {|
  fn_return := tbool;
  fn_callconv := cc_default;
  fn_params := ((_value, tulong) :: (_out, (tptr tulong)) :: nil);
  fn_vars := nil;
  fn_temps := ((_converted, tbool) :: nil);
  fn_body :=
(Ssequence
  (Sset _converted (Ecast (Econst_int (Int.repr 0) tint) tbool))
  (Ssequence
    (Sifthenelse (Ebinop Ole (Etempvar _value tulong)
                   (Ecast (Econst_long (Int64.repr (-1)) tulong) tulong)
                   tint)
      (Ssequence
        (Sifthenelse (Ebinop One (Etempvar _out (tptr tulong))
                       (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid))
                       tint)
          (Sassign (Ederef (Etempvar _out (tptr tulong)) tulong)
            (Ecast (Etempvar _value tulong) tulong))
          Sskip)
        (Sset _converted (Ecast (Econst_int (Int.repr 1) tint) tbool)))
      Sskip)
    (Sreturn (Some (Etempvar _converted tbool)))))
|}.

Definition f_ssz_internal_bits_to_bytes := {|
  fn_return := tbool;
  fn_callconv := cc_default;
  fn_params := ((_bit_count, tulong) :: (_out_bytes, (tptr tulong)) :: nil);
  fn_vars := nil;
  fn_temps := ((_bytes_u64, tulong) :: (_t'1, tbool) :: nil);
  fn_body :=
(Ssequence
  (Sset _bytes_u64
    (Ebinop Odiv (Etempvar _bit_count tulong) (Econst_int (Int.repr 8) tuint)
      tulong))
  (Ssequence
    (Sifthenelse (Ebinop One
                   (Ebinop Omod (Etempvar _bit_count tulong)
                     (Econst_int (Int.repr 8) tuint) tulong)
                   (Econst_int (Int.repr 0) tuint) tint)
      (Sset _bytes_u64
        (Ebinop Oadd (Etempvar _bytes_u64 tulong)
          (Econst_int (Int.repr 1) tint) tulong))
      Sskip)
    (Ssequence
      (Scall (Some _t'1)
        (Evar _ssz_internal_u64_to_size (Tfunction
                                          (tulong :: (tptr tulong) :: nil)
                                          tbool cc_default))
        ((Etempvar _bytes_u64 tulong) ::
         (Etempvar _out_bytes (tptr tulong)) :: nil))
      (Sreturn (Some (Etempvar _t'1 tbool))))))
|}.

Definition f_ssz_internal_selector_allowed := {|
  fn_return := tbool;
  fn_callconv := cc_default;
  fn_params := ((_selector, tuchar) :: (_allowed_selectors, (tptr tuchar)) ::
                (_allowed_selector_count, tuint) :: nil);
  fn_vars := nil;
  fn_temps := ((_allowed, tbool) :: (_i, tuint) :: (_t'1, tuchar) :: nil);
  fn_body :=
(Ssequence
  (Sset _allowed (Ecast (Econst_int (Int.repr 0) tint) tbool))
  (Ssequence
    (Ssequence
      (Sset _i (Econst_int (Int.repr 0) tuint))
      (Sloop
        (Ssequence
          (Sifthenelse (Ebinop Olt (Etempvar _i tuint)
                         (Etempvar _allowed_selector_count tuint) tint)
            Sskip
            Sbreak)
          (Ssequence
            (Sset _t'1
              (Ederef
                (Ebinop Oadd (Etempvar _allowed_selectors (tptr tuchar))
                  (Etempvar _i tuint) (tptr tuchar)) tuchar))
            (Sifthenelse (Ebinop Oeq (Etempvar _t'1 tuchar)
                           (Etempvar _selector tuchar) tint)
              (Ssequence
                (Sset _allowed (Ecast (Econst_int (Int.repr 1) tint) tbool))
                Sbreak)
              Sskip)))
        (Sset _i
          (Ebinop Oadd (Etempvar _i tuint) (Econst_int (Int.repr 1) tint)
            tuint))))
    (Sreturn (Some (Etempvar _allowed tbool)))))
|}.

Definition f_ssz_internal_validate_compatible_union_schema := {|
  fn_return := tint;
  fn_callconv := cc_default;
  fn_params := ((_allowed_selectors, (tptr tuchar)) ::
                (_allowed_selector_count, tuint) :: nil);
  fn_vars := nil;
  fn_temps := ((_err, tint) :: (_i, tuint) :: (_t'2, tint) :: (_t'1, tint) ::
               (_t'4, tuchar) :: (_t'3, tuchar) :: nil);
  fn_body :=
(Ssequence
  (Sset _err (Econst_int (Int.repr 0) tint))
  (Ssequence
    (Ssequence
      (Sifthenelse (Ebinop Oeq (Etempvar _allowed_selectors (tptr tuchar))
                     (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid))
                     tint)
        (Sset _t'2 (Econst_int (Int.repr 1) tint))
        (Sset _t'2
          (Ecast
            (Ebinop Oeq (Etempvar _allowed_selector_count tuint)
              (Econst_int (Int.repr 0) tuint) tint) tbool)))
      (Sifthenelse (Etempvar _t'2 tint)
        (Sset _err (Econst_int (Int.repr 5) tint))
        (Ssequence
          (Sset _i (Econst_int (Int.repr 0) tuint))
          (Sloop
            (Ssequence
              (Sifthenelse (Ebinop Olt (Etempvar _i tuint)
                             (Etempvar _allowed_selector_count tuint) tint)
                Sskip
                Sbreak)
              (Ssequence
                (Ssequence
                  (Sset _t'3
                    (Ederef
                      (Ebinop Oadd
                        (Etempvar _allowed_selectors (tptr tuchar))
                        (Etempvar _i tuint) (tptr tuchar)) tuchar))
                  (Sifthenelse (Ebinop Oeq (Etempvar _t'3 tuchar)
                                 (Econst_int (Int.repr 0) tuint) tint)
                    (Sset _t'1 (Econst_int (Int.repr 1) tint))
                    (Ssequence
                      (Sset _t'4
                        (Ederef
                          (Ebinop Oadd
                            (Etempvar _allowed_selectors (tptr tuchar))
                            (Etempvar _i tuint) (tptr tuchar)) tuchar))
                      (Sset _t'1
                        (Ecast
                          (Ebinop Ogt (Etempvar _t'4 tuchar)
                            (Econst_int (Int.repr 127) tuint) tint) tbool)))))
                (Sifthenelse (Etempvar _t'1 tint)
                  (Ssequence
                    (Sset _err (Econst_int (Int.repr 5) tint))
                    Sbreak)
                  Sskip)))
            (Sset _i
              (Ebinop Oadd (Etempvar _i tuint) (Econst_int (Int.repr 1) tint)
                tuint))))))
    (Sreturn (Some (Etempvar _err tint)))))
|}.

Definition f_ssz_internal_prepare_output := {|
  fn_return := tint;
  fn_callconv := cc_default;
  fn_params := ((_required, tulong) :: (_out, (tptr tuchar)) ::
                (_out_cap, tulong) :: (_out_len, (tptr tulong)) :: nil);
  fn_vars := nil;
  fn_temps := ((_err, tint) :: (_t'1, tint) :: nil);
  fn_body :=
(Ssequence
  (Sset _err (Econst_int (Int.repr 0) tint))
  (Ssequence
    (Sifthenelse (Ebinop Oeq (Etempvar _out_len (tptr tulong))
                   (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid)) tint)
      (Sset _err (Econst_int (Int.repr 1) tint))
      (Ssequence
        (Sassign (Ederef (Etempvar _out_len (tptr tulong)) tulong)
          (Etempvar _required tulong))
        (Ssequence
          (Sifthenelse (Ebinop One (Etempvar _out (tptr tuchar))
                         (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid))
                         tint)
            (Sset _t'1
              (Ecast
                (Ebinop Olt (Etempvar _out_cap tulong)
                  (Etempvar _required tulong) tint) tbool))
            (Sset _t'1 (Econst_int (Int.repr 0) tint)))
          (Sifthenelse (Etempvar _t'1 tint)
            (Sset _err (Econst_int (Int.repr 2) tint))
            Sskip))))
    (Sreturn (Some (Etempvar _err tint)))))
|}.

Definition f_ssz_serialize_uint8 := {|
  fn_return := tint;
  fn_callconv := cc_default;
  fn_params := ((_value, tuchar) :: (_out, (tptr tuchar)) :: nil);
  fn_vars := nil;
  fn_temps := ((_err, tint) :: nil);
  fn_body :=
(Ssequence
  (Sset _err (Econst_int (Int.repr 0) tint))
  (Ssequence
    (Sifthenelse (Ebinop Oeq (Etempvar _out (tptr tuchar))
                   (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid)) tint)
      (Sset _err (Econst_int (Int.repr 1) tint))
      (Sassign
        (Ederef
          (Ebinop Oadd (Etempvar _out (tptr tuchar))
            (Econst_int (Int.repr 0) tint) (tptr tuchar)) tuchar)
        (Etempvar _value tuchar)))
    (Sreturn (Some (Etempvar _err tint)))))
|}.

Definition f_ssz_serialize_uint16 := {|
  fn_return := tint;
  fn_callconv := cc_default;
  fn_params := ((_value, tushort) :: (_out, (tptr tuchar)) :: nil);
  fn_vars := nil;
  fn_temps := ((_err, tint) :: nil);
  fn_body :=
(Ssequence
  (Sset _err (Econst_int (Int.repr 0) tint))
  (Ssequence
    (Sifthenelse (Ebinop Oeq (Etempvar _out (tptr tuchar))
                   (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid)) tint)
      (Sset _err (Econst_int (Int.repr 1) tint))
      (Scall None
        (Evar _ssz_internal_write_u16_le (Tfunction
                                           ((tptr tuchar) :: tushort :: nil)
                                           tvoid cc_default))
        ((Etempvar _out (tptr tuchar)) :: (Etempvar _value tushort) :: nil)))
    (Sreturn (Some (Etempvar _err tint)))))
|}.

Definition f_ssz_serialize_uint32 := {|
  fn_return := tint;
  fn_callconv := cc_default;
  fn_params := ((_value, tuint) :: (_out, (tptr tuchar)) :: nil);
  fn_vars := nil;
  fn_temps := ((_err, tint) :: nil);
  fn_body :=
(Ssequence
  (Sset _err (Econst_int (Int.repr 0) tint))
  (Ssequence
    (Sifthenelse (Ebinop Oeq (Etempvar _out (tptr tuchar))
                   (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid)) tint)
      (Sset _err (Econst_int (Int.repr 1) tint))
      (Scall None
        (Evar _ssz_internal_write_u32_le (Tfunction
                                           ((tptr tuchar) :: tuint :: nil)
                                           tvoid cc_default))
        ((Etempvar _out (tptr tuchar)) :: (Etempvar _value tuint) :: nil)))
    (Sreturn (Some (Etempvar _err tint)))))
|}.

Definition f_ssz_serialize_uint64 := {|
  fn_return := tint;
  fn_callconv := cc_default;
  fn_params := ((_value, tulong) :: (_out, (tptr tuchar)) :: nil);
  fn_vars := nil;
  fn_temps := ((_err, tint) :: nil);
  fn_body :=
(Ssequence
  (Sset _err (Econst_int (Int.repr 0) tint))
  (Ssequence
    (Sifthenelse (Ebinop Oeq (Etempvar _out (tptr tuchar))
                   (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid)) tint)
      (Sset _err (Econst_int (Int.repr 1) tint))
      (Scall None
        (Evar _ssz_internal_write_u64_le (Tfunction
                                           ((tptr tuchar) :: tulong :: nil)
                                           tvoid cc_default))
        ((Etempvar _out (tptr tuchar)) :: (Etempvar _value tulong) :: nil)))
    (Sreturn (Some (Etempvar _err tint)))))
|}.

Definition f_ssz_serialize_uint128 := {|
  fn_return := tint;
  fn_callconv := cc_default;
  fn_params := ((_value, (tptr tuchar)) :: (_value_len, tulong) ::
                (_out, (tptr tuchar)) :: nil);
  fn_vars := nil;
  fn_temps := ((_err, tint) :: (_t'1, tint) :: nil);
  fn_body :=
(Ssequence
  (Sset _err (Econst_int (Int.repr 0) tint))
  (Ssequence
    (Ssequence
      (Sifthenelse (Ebinop Oeq (Etempvar _value (tptr tuchar))
                     (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid))
                     tint)
        (Sset _t'1 (Econst_int (Int.repr 1) tint))
        (Sset _t'1
          (Ecast
            (Ebinop Oeq (Etempvar _out (tptr tuchar))
              (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid)) tint)
            tbool)))
      (Sifthenelse (Etempvar _t'1 tint)
        (Sset _err (Econst_int (Int.repr 1) tint))
        (Sifthenelse (Ebinop One (Etempvar _value_len tulong)
                       (Econst_int (Int.repr 16) tuint) tint)
          (Sset _err (Econst_int (Int.repr 6) tint))
          (Scall None
            (Evar _memcpy (Tfunction
                            ((tptr tvoid) :: (tptr tvoid) :: tulong :: nil)
                            (tptr tvoid) cc_default))
            ((Etempvar _out (tptr tuchar)) ::
             (Etempvar _value (tptr tuchar)) ::
             (Econst_int (Int.repr 16) tuint) :: nil)))))
    (Sreturn (Some (Etempvar _err tint)))))
|}.

Definition f_ssz_serialize_uint256 := {|
  fn_return := tint;
  fn_callconv := cc_default;
  fn_params := ((_value, (tptr tuchar)) :: (_value_len, tulong) ::
                (_out, (tptr tuchar)) :: nil);
  fn_vars := nil;
  fn_temps := ((_err, tint) :: (_t'1, tint) :: nil);
  fn_body :=
(Ssequence
  (Sset _err (Econst_int (Int.repr 0) tint))
  (Ssequence
    (Ssequence
      (Sifthenelse (Ebinop Oeq (Etempvar _value (tptr tuchar))
                     (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid))
                     tint)
        (Sset _t'1 (Econst_int (Int.repr 1) tint))
        (Sset _t'1
          (Ecast
            (Ebinop Oeq (Etempvar _out (tptr tuchar))
              (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid)) tint)
            tbool)))
      (Sifthenelse (Etempvar _t'1 tint)
        (Sset _err (Econst_int (Int.repr 1) tint))
        (Sifthenelse (Ebinop One (Etempvar _value_len tulong)
                       (Econst_int (Int.repr 32) tuint) tint)
          (Sset _err (Econst_int (Int.repr 6) tint))
          (Scall None
            (Evar _memcpy (Tfunction
                            ((tptr tvoid) :: (tptr tvoid) :: tulong :: nil)
                            (tptr tvoid) cc_default))
            ((Etempvar _out (tptr tuchar)) ::
             (Etempvar _value (tptr tuchar)) ::
             (Econst_int (Int.repr 32) tuint) :: nil)))))
    (Sreturn (Some (Etempvar _err tint)))))
|}.

Definition f_ssz_serialize_boolean := {|
  fn_return := tint;
  fn_callconv := cc_default;
  fn_params := ((_value, tuchar) :: (_out, (tptr tuchar)) :: nil);
  fn_vars := nil;
  fn_temps := ((_err, tint) :: nil);
  fn_body :=
(Ssequence
  (Sset _err (Econst_int (Int.repr 0) tint))
  (Ssequence
    (Sifthenelse (Ebinop Oeq (Etempvar _out (tptr tuchar))
                   (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid)) tint)
      (Sset _err (Econst_int (Int.repr 1) tint))
      (Sifthenelse (Ebinop Ogt (Etempvar _value tuchar)
                     (Econst_int (Int.repr 1) tuint) tint)
        (Sset _err (Econst_int (Int.repr 6) tint))
        (Sassign
          (Ederef
            (Ebinop Oadd (Etempvar _out (tptr tuchar))
              (Econst_int (Int.repr 0) tint) (tptr tuchar)) tuchar)
          (Etempvar _value tuchar))))
    (Sreturn (Some (Etempvar _err tint)))))
|}.

Definition f_ssz_serialize_bitvector := {|
  fn_return := tint;
  fn_callconv := cc_default;
  fn_params := ((_bits_le, (tptr tuchar)) :: (_bits_le_len, tulong) ::
                (_bit_count, tulong) :: (_out, (tptr tuchar)) ::
                (_out_cap, tulong) :: (_out_len, (tptr tulong)) :: nil);
  fn_vars := ((_byte_count, tulong) :: nil);
  fn_temps := ((_err, tint) :: (_mask, tuchar) :: (_t'4, tbool) ::
               (_t'3, tint) :: (_t'2, tint) :: (_t'1, tint) ::
               (_t'9, tulong) :: (_t'8, tuchar) :: (_t'7, tulong) ::
               (_t'6, tulong) :: (_t'5, tulong) :: nil);
  fn_body :=
(Ssequence
  (Sassign (Evar _byte_count tulong) (Econst_int (Int.repr 0) tuint))
  (Ssequence
    (Sset _err (Econst_int (Int.repr 0) tint))
    (Ssequence
      (Sifthenelse (Ebinop Oeq (Etempvar _bit_count tulong)
                     (Econst_int (Int.repr 0) tuint) tint)
        (Sset _err (Econst_int (Int.repr 5) tint))
        (Ssequence
          (Scall (Some _t'4)
            (Evar _ssz_internal_bits_to_bytes (Tfunction
                                                (tulong :: (tptr tulong) ::
                                                 nil) tbool cc_default))
            ((Etempvar _bit_count tulong) ::
             (Eaddrof (Evar _byte_count tulong) (tptr tulong)) :: nil))
          (Sifthenelse (Eunop Onotbool (Etempvar _t'4 tbool) tint)
            (Sset _err (Econst_int (Int.repr 3) tint))
            (Ssequence
              (Sifthenelse (Ebinop Oeq (Etempvar _bits_le (tptr tuchar))
                             (Ecast (Econst_int (Int.repr 0) tint)
                               (tptr tvoid)) tint)
                (Sset _t'3 (Econst_int (Int.repr 1) tint))
                (Ssequence
                  (Sset _t'9 (Evar _byte_count tulong))
                  (Sset _t'3
                    (Ecast
                      (Ebinop Olt (Etempvar _bits_le_len tulong)
                        (Etempvar _t'9 tulong) tint) tbool))))
              (Sifthenelse (Etempvar _t'3 tint)
                (Sset _err (Econst_int (Int.repr 1) tint))
                (Ssequence
                  (Sifthenelse (Ebinop One
                                 (Ebinop Omod (Etempvar _bit_count tulong)
                                   (Econst_int (Int.repr 8) tuint) tulong)
                                 (Econst_int (Int.repr 0) tuint) tint)
                    (Ssequence
                      (Sset _mask
                        (Ecast
                          (Ecast
                            (Ebinop Osub
                              (Ebinop Oshl (Econst_int (Int.repr 1) tuint)
                                (Ebinop Omod (Etempvar _bit_count tulong)
                                  (Econst_int (Int.repr 8) tuint) tulong)
                                tuint) (Econst_int (Int.repr 1) tuint) tuint)
                            tuchar) tuchar))
                      (Ssequence
                        (Sset _t'7 (Evar _byte_count tulong))
                        (Ssequence
                          (Sset _t'8
                            (Ederef
                              (Ebinop Oadd (Etempvar _bits_le (tptr tuchar))
                                (Ebinop Osub (Etempvar _t'7 tulong)
                                  (Econst_int (Int.repr 1) tuint) tulong)
                                (tptr tuchar)) tuchar))
                          (Sifthenelse (Ebinop One
                                         (Ebinop Oand (Etempvar _t'8 tuchar)
                                           (Ecast
                                             (Eunop Onotint
                                               (Etempvar _mask tuchar) tint)
                                             tuchar) tint)
                                         (Econst_int (Int.repr 0) tuint)
                                         tint)
                            (Sset _err (Econst_int (Int.repr 6) tint))
                            Sskip))))
                    Sskip)
                  (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                                 (Econst_int (Int.repr 0) tint) tint)
                    (Ssequence
                      (Ssequence
                        (Ssequence
                          (Sset _t'6 (Evar _byte_count tulong))
                          (Scall (Some _t'1)
                            (Evar _ssz_internal_prepare_output (Tfunction
                                                                 (tulong ::
                                                                  (tptr tuchar) ::
                                                                  tulong ::
                                                                  (tptr tulong) ::
                                                                  nil) tint
                                                                 cc_default))
                            ((Etempvar _t'6 tulong) ::
                             (Etempvar _out (tptr tuchar)) ::
                             (Etempvar _out_cap tulong) ::
                             (Etempvar _out_len (tptr tulong)) :: nil)))
                        (Sset _err (Etempvar _t'1 tint)))
                      (Ssequence
                        (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                                       (Econst_int (Int.repr 0) tint) tint)
                          (Sset _t'2
                            (Ecast
                              (Ebinop One (Etempvar _out (tptr tuchar))
                                (Ecast (Econst_int (Int.repr 0) tint)
                                  (tptr tvoid)) tint) tbool))
                          (Sset _t'2 (Econst_int (Int.repr 0) tint)))
                        (Sifthenelse (Etempvar _t'2 tint)
                          (Ssequence
                            (Sset _t'5 (Evar _byte_count tulong))
                            (Scall None
                              (Evar _memcpy (Tfunction
                                              ((tptr tvoid) ::
                                               (tptr tvoid) :: tulong :: nil)
                                              (tptr tvoid) cc_default))
                              ((Etempvar _out (tptr tuchar)) ::
                               (Etempvar _bits_le (tptr tuchar)) ::
                               (Etempvar _t'5 tulong) :: nil)))
                          Sskip)))
                    Sskip)))))))
      (Sreturn (Some (Etempvar _err tint))))))
|}.

Definition f_ssz_serialize_bitlist := {|
  fn_return := tint;
  fn_callconv := cc_default;
  fn_params := ((_bits_le, (tptr tuchar)) :: (_bits_le_len, tulong) ::
                (_bit_len, tulong) :: (_bit_limit, tulong) ::
                (_out, (tptr tuchar)) :: (_out_cap, tulong) ::
                (_out_len, (tptr tulong)) :: nil);
  fn_vars := ((_data_bytes, tulong) :: (_delimiter_byte, tulong) ::
              (_required, tulong) :: nil);
  fn_temps := ((_err, tint) :: (_mask, tuchar) :: (_delimiter_bit, tuchar) ::
               (_t'8, tint) :: (_t'7, tbool) :: (_t'6, tbool) ::
               (_t'5, tint) :: (_t'4, tbool) :: (_t'3, tint) ::
               (_t'2, tint) :: (_t'1, tint) :: (_t'20, tulong) ::
               (_t'19, tulong) :: (_t'18, tuchar) :: (_t'17, tulong) ::
               (_t'16, tulong) :: (_t'15, tulong) :: (_t'14, tulong) ::
               (_t'13, tulong) :: (_t'12, tulong) :: (_t'11, tuchar) ::
               (_t'10, tulong) :: (_t'9, tulong) :: nil);
  fn_body :=
(Ssequence
  (Sassign (Evar _data_bytes tulong) (Econst_int (Int.repr 0) tuint))
  (Ssequence
    (Sassign (Evar _delimiter_byte tulong) (Econst_int (Int.repr 0) tuint))
    (Ssequence
      (Sassign (Evar _required tulong) (Econst_int (Int.repr 0) tuint))
      (Ssequence
        (Sset _err (Econst_int (Int.repr 0) tint))
        (Ssequence
          (Ssequence
            (Sifthenelse (Ebinop One (Etempvar _bit_limit tulong)
                           (Econst_long (Int64.repr (-1)) tulong) tint)
              (Sset _t'8
                (Ecast
                  (Ebinop Ogt (Etempvar _bit_len tulong)
                    (Etempvar _bit_limit tulong) tint) tbool))
              (Sset _t'8 (Econst_int (Int.repr 0) tint)))
            (Sifthenelse (Etempvar _t'8 tint)
              (Sset _err (Econst_int (Int.repr 4) tint))
              (Ssequence
                (Scall (Some _t'7)
                  (Evar _ssz_internal_bits_to_bytes (Tfunction
                                                      (tulong ::
                                                       (tptr tulong) :: nil)
                                                      tbool cc_default))
                  ((Etempvar _bit_len tulong) ::
                   (Eaddrof (Evar _data_bytes tulong) (tptr tulong)) :: nil))
                (Sifthenelse (Eunop Onotbool (Etempvar _t'7 tbool) tint)
                  (Sset _err (Econst_int (Int.repr 3) tint))
                  (Ssequence
                    (Ssequence
                      (Scall (Some _t'4)
                        (Evar _ssz_internal_u64_to_size (Tfunction
                                                          (tulong ::
                                                           (tptr tulong) ::
                                                           nil) tbool
                                                          cc_default))
                        ((Ebinop Odiv (Etempvar _bit_len tulong)
                           (Econst_int (Int.repr 8) tuint) tulong) ::
                         (Eaddrof (Evar _delimiter_byte tulong)
                           (tptr tulong)) :: nil))
                      (Sifthenelse (Eunop Onotbool (Etempvar _t'4 tbool)
                                     tint)
                        (Sset _t'5 (Econst_int (Int.repr 1) tint))
                        (Ssequence
                          (Ssequence
                            (Sset _t'20 (Evar _delimiter_byte tulong))
                            (Scall (Some _t'6)
                              (Evar _ssz_internal_add_overflow_size (Tfunction
                                                                    (tulong ::
                                                                    tulong ::
                                                                    (tptr tulong) ::
                                                                    nil)
                                                                    tbool
                                                                    cc_default))
                              ((Etempvar _t'20 tulong) ::
                               (Econst_int (Int.repr 1) tuint) ::
                               (Eaddrof (Evar _required tulong)
                                 (tptr tulong)) :: nil)))
                          (Sset _t'5 (Ecast (Etempvar _t'6 tbool) tbool)))))
                    (Sifthenelse (Etempvar _t'5 tint)
                      (Sset _err (Econst_int (Int.repr 3) tint))
                      (Ssequence
                        (Ssequence
                          (Sset _t'16 (Evar _data_bytes tulong))
                          (Sifthenelse (Ebinop One (Etempvar _t'16 tulong)
                                         (Econst_int (Int.repr 0) tuint)
                                         tint)
                            (Ssequence
                              (Sifthenelse (Ebinop Oeq
                                             (Etempvar _bits_le (tptr tuchar))
                                             (Ecast
                                               (Econst_int (Int.repr 0) tint)
                                               (tptr tvoid)) tint)
                                (Sset _t'1 (Econst_int (Int.repr 1) tint))
                                (Ssequence
                                  (Sset _t'19 (Evar _data_bytes tulong))
                                  (Sset _t'1
                                    (Ecast
                                      (Ebinop Olt
                                        (Etempvar _bits_le_len tulong)
                                        (Etempvar _t'19 tulong) tint) tbool))))
                              (Sifthenelse (Etempvar _t'1 tint)
                                (Sset _err (Econst_int (Int.repr 1) tint))
                                (Sifthenelse (Ebinop One
                                               (Ebinop Omod
                                                 (Etempvar _bit_len tulong)
                                                 (Econst_int (Int.repr 8) tuint)
                                                 tulong)
                                               (Econst_int (Int.repr 0) tuint)
                                               tint)
                                  (Ssequence
                                    (Sset _mask
                                      (Ecast
                                        (Ecast
                                          (Ebinop Osub
                                            (Ebinop Oshl
                                              (Econst_int (Int.repr 1) tuint)
                                              (Ebinop Omod
                                                (Etempvar _bit_len tulong)
                                                (Econst_int (Int.repr 8) tuint)
                                                tulong) tuint)
                                            (Econst_int (Int.repr 1) tuint)
                                            tuint) tuchar) tuchar))
                                    (Ssequence
                                      (Sset _t'17 (Evar _data_bytes tulong))
                                      (Ssequence
                                        (Sset _t'18
                                          (Ederef
                                            (Ebinop Oadd
                                              (Etempvar _bits_le (tptr tuchar))
                                              (Ebinop Osub
                                                (Etempvar _t'17 tulong)
                                                (Econst_int (Int.repr 1) tuint)
                                                tulong) (tptr tuchar))
                                            tuchar))
                                        (Sifthenelse (Ebinop One
                                                       (Ebinop Oand
                                                         (Etempvar _t'18 tuchar)
                                                         (Ecast
                                                           (Eunop Onotint
                                                             (Etempvar _mask tuchar)
                                                             tint) tuchar)
                                                         tint)
                                                       (Econst_int (Int.repr 0) tuint)
                                                       tint)
                                          (Sset _err
                                            (Econst_int (Int.repr 6) tint))
                                          Sskip))))
                                  Sskip)))
                            Sskip))
                        (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                                       (Econst_int (Int.repr 0) tint) tint)
                          (Ssequence
                            (Ssequence
                              (Ssequence
                                (Sset _t'15 (Evar _required tulong))
                                (Scall (Some _t'2)
                                  (Evar _ssz_internal_prepare_output 
                                  (Tfunction
                                    (tulong :: (tptr tuchar) :: tulong ::
                                     (tptr tulong) :: nil) tint cc_default))
                                  ((Etempvar _t'15 tulong) ::
                                   (Etempvar _out (tptr tuchar)) ::
                                   (Etempvar _out_cap tulong) ::
                                   (Etempvar _out_len (tptr tulong)) :: nil)))
                              (Sset _err (Etempvar _t'2 tint)))
                            (Ssequence
                              (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                                             (Econst_int (Int.repr 0) tint)
                                             tint)
                                (Sset _t'3
                                  (Ecast
                                    (Ebinop One (Etempvar _out (tptr tuchar))
                                      (Ecast (Econst_int (Int.repr 0) tint)
                                        (tptr tvoid)) tint) tbool))
                                (Sset _t'3 (Econst_int (Int.repr 0) tint)))
                              (Sifthenelse (Etempvar _t'3 tint)
                                (Ssequence
                                  (Sset _delimiter_bit
                                    (Ecast
                                      (Ecast
                                        (Ebinop Oshl
                                          (Econst_int (Int.repr 1) tuint)
                                          (Ebinop Omod
                                            (Etempvar _bit_len tulong)
                                            (Econst_int (Int.repr 8) tuint)
                                            tulong) tuint) tuchar) tuchar))
                                  (Ssequence
                                    (Ssequence
                                      (Sset _t'14 (Evar _required tulong))
                                      (Scall None
                                        (Evar _memset (Tfunction
                                                        ((tptr tvoid) ::
                                                         tint :: tulong ::
                                                         nil) (tptr tvoid)
                                                        cc_default))
                                        ((Etempvar _out (tptr tuchar)) ::
                                         (Econst_int (Int.repr 0) tint) ::
                                         (Etempvar _t'14 tulong) :: nil)))
                                    (Ssequence
                                      (Ssequence
                                        (Sset _t'12
                                          (Evar _data_bytes tulong))
                                        (Sifthenelse (Ebinop One
                                                       (Etempvar _t'12 tulong)
                                                       (Econst_int (Int.repr 0) tuint)
                                                       tint)
                                          (Ssequence
                                            (Sset _t'13
                                              (Evar _data_bytes tulong))
                                            (Scall None
                                              (Evar _memcpy (Tfunction
                                                              ((tptr tvoid) ::
                                                               (tptr tvoid) ::
                                                               tulong :: nil)
                                                              (tptr tvoid)
                                                              cc_default))
                                              ((Etempvar _out (tptr tuchar)) ::
                                               (Etempvar _bits_le (tptr tuchar)) ::
                                               (Etempvar _t'13 tulong) ::
                                               nil)))
                                          Sskip))
                                      (Ssequence
                                        (Sset _t'9
                                          (Evar _delimiter_byte tulong))
                                        (Ssequence
                                          (Sset _t'10
                                            (Evar _delimiter_byte tulong))
                                          (Ssequence
                                            (Sset _t'11
                                              (Ederef
                                                (Ebinop Oadd
                                                  (Etempvar _out (tptr tuchar))
                                                  (Etempvar _t'10 tulong)
                                                  (tptr tuchar)) tuchar))
                                            (Sassign
                                              (Ederef
                                                (Ebinop Oadd
                                                  (Etempvar _out (tptr tuchar))
                                                  (Etempvar _t'9 tulong)
                                                  (tptr tuchar)) tuchar)
                                              (Ecast
                                                (Ebinop Oor
                                                  (Etempvar _t'11 tuchar)
                                                  (Etempvar _delimiter_bit tuchar)
                                                  tint) tuchar))))))))
                                Sskip)))
                          Sskip))))))))
          (Sreturn (Some (Etempvar _err tint))))))))
|}.

Definition f_ssz_serialize_vector_fixed := {|
  fn_return := tint;
  fn_callconv := cc_default;
  fn_params := ((_elements, (tptr tuchar)) :: (_element_count, tulong) ::
                (_element_size, tulong) :: (_out, (tptr tuchar)) ::
                (_out_cap, tulong) :: (_out_len, (tptr tulong)) :: nil);
  fn_vars := ((_required, tulong) :: nil);
  fn_temps := ((_err, tint) :: (_t'6, tbool) :: (_t'5, tbool) ::
               (_t'4, tint) :: (_t'3, tint) :: (_t'2, tint) ::
               (_t'1, tint) :: (_t'11, tulong) :: (_t'10, tulong) ::
               (_t'9, tulong) :: (_t'8, tulong) :: (_t'7, tulong) :: nil);
  fn_body :=
(Ssequence
  (Sassign (Evar _required tulong) (Econst_int (Int.repr 0) tuint))
  (Ssequence
    (Sset _err (Econst_int (Int.repr 0) tint))
    (Ssequence
      (Sifthenelse (Ebinop Oeq (Etempvar _element_count tulong)
                     (Econst_int (Int.repr 0) tuint) tint)
        (Sset _err (Econst_int (Int.repr 5) tint))
        (Sifthenelse (Ebinop Oeq (Etempvar _element_size tulong)
                       (Econst_int (Int.repr 0) tuint) tint)
          (Sset _err (Econst_int (Int.repr 5) tint))
          (Ssequence
            (Scall (Some _t'6)
              (Evar _ssz_internal_u64_to_size (Tfunction
                                                (tulong :: (tptr tulong) ::
                                                 nil) tbool cc_default))
              ((Etempvar _element_count tulong) ::
               (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid)) :: nil))
            (Sifthenelse (Eunop Onotbool (Etempvar _t'6 tbool) tint)
              (Sset _err (Econst_int (Int.repr 3) tint))
              (Ssequence
                (Scall (Some _t'5)
                  (Evar _ssz_internal_mul_overflow_size (Tfunction
                                                          (tulong ::
                                                           tulong ::
                                                           (tptr tulong) ::
                                                           nil) tbool
                                                          cc_default))
                  ((Ecast (Etempvar _element_count tulong) tulong) ::
                   (Etempvar _element_size tulong) ::
                   (Eaddrof (Evar _required tulong) (tptr tulong)) :: nil))
                (Sifthenelse (Etempvar _t'5 tbool)
                  (Sset _err (Econst_int (Int.repr 3) tint))
                  (Ssequence
                    (Sset _t'7 (Evar _required tulong))
                    (Sifthenelse (Ebinop Ogt (Etempvar _t'7 tulong)
                                   (Econst_int (Int.repr (-1)) tuint) tint)
                      (Sset _err (Econst_int (Int.repr 3) tint))
                      (Ssequence
                        (Ssequence
                          (Sset _t'11 (Evar _required tulong))
                          (Sifthenelse (Ebinop One (Etempvar _t'11 tulong)
                                         (Econst_int (Int.repr 0) tuint)
                                         tint)
                            (Sset _t'4
                              (Ecast
                                (Ebinop Oeq
                                  (Etempvar _elements (tptr tuchar))
                                  (Ecast (Econst_int (Int.repr 0) tint)
                                    (tptr tvoid)) tint) tbool))
                            (Sset _t'4 (Econst_int (Int.repr 0) tint))))
                        (Sifthenelse (Etempvar _t'4 tint)
                          (Sset _err (Econst_int (Int.repr 1) tint))
                          (Ssequence
                            (Ssequence
                              (Ssequence
                                (Sset _t'10 (Evar _required tulong))
                                (Scall (Some _t'1)
                                  (Evar _ssz_internal_prepare_output 
                                  (Tfunction
                                    (tulong :: (tptr tuchar) :: tulong ::
                                     (tptr tulong) :: nil) tint cc_default))
                                  ((Etempvar _t'10 tulong) ::
                                   (Etempvar _out (tptr tuchar)) ::
                                   (Etempvar _out_cap tulong) ::
                                   (Etempvar _out_len (tptr tulong)) :: nil)))
                              (Sset _err (Etempvar _t'1 tint)))
                            (Ssequence
                              (Ssequence
                                (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                                               (Econst_int (Int.repr 0) tint)
                                               tint)
                                  (Sset _t'2
                                    (Ecast
                                      (Ebinop One
                                        (Etempvar _out (tptr tuchar))
                                        (Ecast (Econst_int (Int.repr 0) tint)
                                          (tptr tvoid)) tint) tbool))
                                  (Sset _t'2 (Econst_int (Int.repr 0) tint)))
                                (Sifthenelse (Etempvar _t'2 tint)
                                  (Ssequence
                                    (Sset _t'9 (Evar _required tulong))
                                    (Sset _t'3
                                      (Ecast
                                        (Ebinop One (Etempvar _t'9 tulong)
                                          (Econst_int (Int.repr 0) tuint)
                                          tint) tbool)))
                                  (Sset _t'3 (Econst_int (Int.repr 0) tint))))
                              (Sifthenelse (Etempvar _t'3 tint)
                                (Ssequence
                                  (Sset _t'8 (Evar _required tulong))
                                  (Scall None
                                    (Evar _memcpy (Tfunction
                                                    ((tptr tvoid) ::
                                                     (tptr tvoid) ::
                                                     tulong :: nil)
                                                    (tptr tvoid) cc_default))
                                    ((Etempvar _out (tptr tuchar)) ::
                                     (Etempvar _elements (tptr tuchar)) ::
                                     (Etempvar _t'8 tulong) :: nil)))
                                Sskip)))))))))))))
      (Sreturn (Some (Etempvar _err tint))))))
|}.

Definition f_ssz_serialize_vector_variable := {|
  fn_return := tint;
  fn_callconv := cc_default;
  fn_params := ((_element_count, tulong) ::
                (_codec, (tptr (Tstruct __435 noattr))) ::
                (_out, (tptr tuchar)) :: (_out_cap, tulong) ::
                (_out_len, (tptr tulong)) :: nil);
  fn_vars := ((_fixed_region, tulong) :: (_total, tulong) ::
              (_encoded_len, tulong) :: (_cursor, tulong) ::
              (_expected_len, tulong) :: (_written, tulong) :: nil);
  fn_temps := ((_err, tint) :: (_i, tulong) :: (_i__1, tulong) ::
               (_t'17, tint) :: (_t'16, tint) :: (_t'15, tbool) ::
               (_t'14, tint) :: (_t'13, tint) :: (_t'12, tint) ::
               (_t'11, tint) :: (_t'10, tint) :: (_t'9, tint) ::
               (_t'8, tint) :: (_t'7, tint) :: (_t'6, tbool) ::
               (_t'5, tbool) :: (_t'4, tbool) :: (_t'3, tint) ::
               (_t'2, tint) :: (_t'1, tint) ::
               (_t'41,
                (tptr (Tfunction
                        ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong ::
                         (tptr tulong) :: nil) tint cc_default))) ::
               (_t'40, tulong) :: (_t'39, (tptr tvoid)) ::
               (_t'38,
                (tptr (Tfunction
                        ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong ::
                         (tptr tulong) :: nil) tint cc_default))) ::
               (_t'37, tulong) :: (_t'36, tulong) :: (_t'35, tulong) ::
               (_t'34, tulong) :: (_t'33, tulong) :: (_t'32, tulong) ::
               (_t'31, (tptr tvoid)) ::
               (_t'30,
                (tptr (Tfunction
                        ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong ::
                         (tptr tulong) :: nil) tint cc_default))) ::
               (_t'29, tulong) :: (_t'28, tulong) :: (_t'27, tulong) ::
               (_t'26, (tptr tvoid)) ::
               (_t'25,
                (tptr (Tfunction
                        ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong ::
                         (tptr tulong) :: nil) tint cc_default))) ::
               (_t'24, tulong) :: (_t'23, tulong) :: (_t'22, tulong) ::
               (_t'21, tulong) :: (_t'20, tulong) :: (_t'19, tulong) ::
               (_t'18, tulong) :: nil);
  fn_body :=
(Ssequence
  (Sassign (Evar _fixed_region tulong) (Econst_int (Int.repr 0) tuint))
  (Ssequence
    (Sassign (Evar _total tulong) (Econst_int (Int.repr 0) tuint))
    (Ssequence
      (Sset _err (Econst_int (Int.repr 0) tint))
      (Ssequence
        (Sifthenelse (Ebinop Oeq (Etempvar _element_count tulong)
                       (Econst_int (Int.repr 0) tuint) tint)
          (Sset _err (Econst_int (Int.repr 5) tint))
          (Ssequence
            (Sifthenelse (Ebinop Oeq
                           (Etempvar _codec (tptr (Tstruct __435 noattr)))
                           (Ecast (Econst_int (Int.repr 0) tint)
                             (tptr tvoid)) tint)
              (Sset _t'7 (Econst_int (Int.repr 1) tint))
              (Ssequence
                (Sset _t'41
                  (Efield
                    (Ederef (Etempvar _codec (tptr (Tstruct __435 noattr)))
                      (Tstruct __435 noattr)) _write
                    (tptr (Tfunction
                            ((tptr tvoid) :: tulong :: (tptr tuchar) ::
                             tulong :: (tptr tulong) :: nil) tint cc_default))))
                (Sset _t'7
                  (Ecast
                    (Ebinop Oeq
                      (Etempvar _t'41 (tptr (Tfunction
                                              ((tptr tvoid) :: tulong ::
                                               (tptr tuchar) :: tulong ::
                                               (tptr tulong) :: nil) tint
                                              cc_default)))
                      (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid))
                      tint) tbool))))
            (Sifthenelse (Etempvar _t'7 tint)
              (Sset _err (Econst_int (Int.repr 1) tint))
              (Ssequence
                (Scall (Some _t'6)
                  (Evar _ssz_internal_u64_to_size (Tfunction
                                                    (tulong ::
                                                     (tptr tulong) :: nil)
                                                    tbool cc_default))
                  ((Etempvar _element_count tulong) ::
                   (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid)) ::
                   nil))
                (Sifthenelse (Eunop Onotbool (Etempvar _t'6 tbool) tint)
                  (Sset _err (Econst_int (Int.repr 3) tint))
                  (Ssequence
                    (Scall (Some _t'5)
                      (Evar _ssz_internal_mul_overflow_size (Tfunction
                                                              (tulong ::
                                                               tulong ::
                                                               (tptr tulong) ::
                                                               nil) tbool
                                                              cc_default))
                      ((Ecast (Etempvar _element_count tulong) tulong) ::
                       (Econst_int (Int.repr 4) tuint) ::
                       (Eaddrof (Evar _fixed_region tulong) (tptr tulong)) ::
                       nil))
                    (Sifthenelse (Etempvar _t'5 tbool)
                      (Sset _err (Econst_int (Int.repr 3) tint))
                      (Ssequence
                        (Sset _t'35 (Evar _fixed_region tulong))
                        (Sifthenelse (Ebinop Ogt (Etempvar _t'35 tulong)
                                       (Econst_int (Int.repr (-1)) tuint)
                                       tint)
                          (Sset _err (Econst_int (Int.repr 3) tint))
                          (Ssequence
                            (Ssequence
                              (Sset _t'40 (Evar _fixed_region tulong))
                              (Sassign (Evar _total tulong)
                                (Etempvar _t'40 tulong)))
                            (Ssequence
                              (Sset _i
                                (Ecast (Econst_int (Int.repr 0) tuint)
                                  tulong))
                              (Sloop
                                (Ssequence
                                  (Ssequence
                                    (Sifthenelse (Ebinop Olt
                                                   (Etempvar _i tulong)
                                                   (Etempvar _element_count tulong)
                                                   tint)
                                      (Sset _t'1
                                        (Ecast
                                          (Ebinop Oeq (Etempvar _err tint)
                                            (Econst_int (Int.repr 0) tint)
                                            tint) tbool))
                                      (Sset _t'1
                                        (Econst_int (Int.repr 0) tint)))
                                    (Sifthenelse (Etempvar _t'1 tint)
                                      Sskip
                                      Sbreak))
                                  (Ssequence
                                    (Sassign (Evar _encoded_len tulong)
                                      (Econst_int (Int.repr 0) tuint))
                                    (Ssequence
                                      (Ssequence
                                        (Ssequence
                                          (Sset _t'38
                                            (Efield
                                              (Ederef
                                                (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                                (Tstruct __435 noattr))
                                              _write
                                              (tptr (Tfunction
                                                      ((tptr tvoid) ::
                                                       tulong ::
                                                       (tptr tuchar) ::
                                                       tulong ::
                                                       (tptr tulong) :: nil)
                                                      tint cc_default))))
                                          (Ssequence
                                            (Sset _t'39
                                              (Efield
                                                (Ederef
                                                  (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                                  (Tstruct __435 noattr))
                                                _ctx (tptr tvoid)))
                                            (Scall (Some _t'2)
                                              (Etempvar _t'38 (tptr (Tfunction
                                                                    ((tptr tvoid) ::
                                                                    tulong ::
                                                                    (tptr tuchar) ::
                                                                    tulong ::
                                                                    (tptr tulong) ::
                                                                    nil) tint
                                                                    cc_default)))
                                              ((Etempvar _t'39 (tptr tvoid)) ::
                                               (Etempvar _i tulong) ::
                                               (Ecast
                                                 (Econst_int (Int.repr 0) tint)
                                                 (tptr tvoid)) ::
                                               (Econst_int (Int.repr 0) tuint) ::
                                               (Eaddrof
                                                 (Evar _encoded_len tulong)
                                                 (tptr tulong)) :: nil))))
                                        (Sset _err (Etempvar _t'2 tint)))
                                      (Ssequence
                                        (Sifthenelse (Ebinop Oeq
                                                       (Etempvar _err tint)
                                                       (Econst_int (Int.repr 0) tint)
                                                       tint)
                                          (Ssequence
                                            (Ssequence
                                              (Sset _t'36
                                                (Evar _total tulong))
                                              (Ssequence
                                                (Sset _t'37
                                                  (Evar _encoded_len tulong))
                                                (Scall (Some _t'4)
                                                  (Evar _ssz_internal_add_overflow_size 
                                                  (Tfunction
                                                    (tulong :: tulong ::
                                                     (tptr tulong) :: nil)
                                                    tbool cc_default))
                                                  ((Etempvar _t'36 tulong) ::
                                                   (Etempvar _t'37 tulong) ::
                                                   (Eaddrof
                                                     (Evar _total tulong)
                                                     (tptr tulong)) :: nil))))
                                            (Sset _t'3
                                              (Ecast (Etempvar _t'4 tbool)
                                                tbool)))
                                          (Sset _t'3
                                            (Econst_int (Int.repr 0) tint)))
                                        (Sifthenelse (Etempvar _t'3 tint)
                                          (Sset _err
                                            (Econst_int (Int.repr 3) tint))
                                          Sskip)))))
                                (Sset _i
                                  (Ebinop Oadd (Etempvar _i tulong)
                                    (Econst_int (Int.repr 1) tint) tulong))))))))))))))
        (Ssequence
          (Ssequence
            (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                           (Econst_int (Int.repr 0) tint) tint)
              (Ssequence
                (Sset _t'34 (Evar _total tulong))
                (Sset _t'8
                  (Ecast
                    (Ebinop Ogt (Etempvar _t'34 tulong)
                      (Econst_int (Int.repr (-1)) tuint) tint) tbool)))
              (Sset _t'8 (Econst_int (Int.repr 0) tint)))
            (Sifthenelse (Etempvar _t'8 tint)
              (Sset _err (Econst_int (Int.repr 3) tint))
              Sskip))
          (Ssequence
            (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                           (Econst_int (Int.repr 0) tint) tint)
              (Ssequence
                (Ssequence
                  (Sset _t'33 (Evar _total tulong))
                  (Scall (Some _t'9)
                    (Evar _ssz_internal_prepare_output (Tfunction
                                                         (tulong ::
                                                          (tptr tuchar) ::
                                                          tulong ::
                                                          (tptr tulong) ::
                                                          nil) tint
                                                         cc_default))
                    ((Etempvar _t'33 tulong) ::
                     (Etempvar _out (tptr tuchar)) ::
                     (Etempvar _out_cap tulong) ::
                     (Etempvar _out_len (tptr tulong)) :: nil)))
                (Sset _err (Etempvar _t'9 tint)))
              Sskip)
            (Ssequence
              (Ssequence
                (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                               (Econst_int (Int.repr 0) tint) tint)
                  (Sset _t'17
                    (Ecast
                      (Ebinop One (Etempvar _out (tptr tuchar))
                        (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid))
                        tint) tbool))
                  (Sset _t'17 (Econst_int (Int.repr 0) tint)))
                (Sifthenelse (Etempvar _t'17 tint)
                  (Ssequence
                    (Ssequence
                      (Sset _t'32 (Evar _fixed_region tulong))
                      (Sassign (Evar _cursor tulong) (Etempvar _t'32 tulong)))
                    (Ssequence
                      (Ssequence
                        (Sset _i__1
                          (Ecast (Econst_int (Int.repr 0) tuint) tulong))
                        (Sloop
                          (Ssequence
                            (Ssequence
                              (Sifthenelse (Ebinop Olt
                                             (Etempvar _i__1 tulong)
                                             (Etempvar _element_count tulong)
                                             tint)
                                (Sset _t'10
                                  (Ecast
                                    (Ebinop Oeq (Etempvar _err tint)
                                      (Econst_int (Int.repr 0) tint) tint)
                                    tbool))
                                (Sset _t'10 (Econst_int (Int.repr 0) tint)))
                              (Sifthenelse (Etempvar _t'10 tint)
                                Sskip
                                Sbreak))
                            (Ssequence
                              (Sset _t'20 (Evar _cursor tulong))
                              (Sifthenelse (Ebinop Ogt
                                             (Etempvar _t'20 tulong)
                                             (Econst_int (Int.repr (-1)) tuint)
                                             tint)
                                (Sset _err (Econst_int (Int.repr 3) tint))
                                (Ssequence
                                  (Sassign (Evar _expected_len tulong)
                                    (Econst_int (Int.repr 0) tuint))
                                  (Ssequence
                                    (Sassign (Evar _written tulong)
                                      (Econst_int (Int.repr 0) tuint))
                                    (Ssequence
                                      (Ssequence
                                        (Ssequence
                                          (Sset _t'30
                                            (Efield
                                              (Ederef
                                                (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                                (Tstruct __435 noattr))
                                              _write
                                              (tptr (Tfunction
                                                      ((tptr tvoid) ::
                                                       tulong ::
                                                       (tptr tuchar) ::
                                                       tulong ::
                                                       (tptr tulong) :: nil)
                                                      tint cc_default))))
                                          (Ssequence
                                            (Sset _t'31
                                              (Efield
                                                (Ederef
                                                  (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                                  (Tstruct __435 noattr))
                                                _ctx (tptr tvoid)))
                                            (Scall (Some _t'11)
                                              (Etempvar _t'30 (tptr (Tfunction
                                                                    ((tptr tvoid) ::
                                                                    tulong ::
                                                                    (tptr tuchar) ::
                                                                    tulong ::
                                                                    (tptr tulong) ::
                                                                    nil) tint
                                                                    cc_default)))
                                              ((Etempvar _t'31 (tptr tvoid)) ::
                                               (Etempvar _i__1 tulong) ::
                                               (Ecast
                                                 (Econst_int (Int.repr 0) tint)
                                                 (tptr tvoid)) ::
                                               (Econst_int (Int.repr 0) tuint) ::
                                               (Eaddrof
                                                 (Evar _expected_len tulong)
                                                 (tptr tulong)) :: nil))))
                                        (Sset _err (Etempvar _t'11 tint)))
                                      (Sifthenelse (Ebinop Oeq
                                                     (Etempvar _err tint)
                                                     (Econst_int (Int.repr 0) tint)
                                                     tint)
                                        (Ssequence
                                          (Ssequence
                                            (Sset _t'29
                                              (Evar _cursor tulong))
                                            (Scall None
                                              (Evar _ssz_internal_write_u32_le 
                                              (Tfunction
                                                ((tptr tuchar) :: tuint ::
                                                 nil) tvoid cc_default))
                                              ((Ebinop Oadd
                                                 (Etempvar _out (tptr tuchar))
                                                 (Ebinop Omul
                                                   (Ecast
                                                     (Etempvar _i__1 tulong)
                                                     tulong)
                                                   (Econst_int (Int.repr 4) tuint)
                                                   tulong) (tptr tuchar)) ::
                                               (Ecast (Etempvar _t'29 tulong)
                                                 tuint) :: nil)))
                                          (Ssequence
                                            (Ssequence
                                              (Ssequence
                                                (Sset _t'25
                                                  (Efield
                                                    (Ederef
                                                      (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                                      (Tstruct __435 noattr))
                                                    _write
                                                    (tptr (Tfunction
                                                            ((tptr tvoid) ::
                                                             tulong ::
                                                             (tptr tuchar) ::
                                                             tulong ::
                                                             (tptr tulong) ::
                                                             nil) tint
                                                            cc_default))))
                                                (Ssequence
                                                  (Sset _t'26
                                                    (Efield
                                                      (Ederef
                                                        (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                                        (Tstruct __435 noattr))
                                                      _ctx (tptr tvoid)))
                                                  (Ssequence
                                                    (Sset _t'27
                                                      (Evar _cursor tulong))
                                                    (Ssequence
                                                      (Sset _t'28
                                                        (Evar _cursor tulong))
                                                      (Scall (Some _t'12)
                                                        (Etempvar _t'25 (tptr 
                                                        (Tfunction
                                                          ((tptr tvoid) ::
                                                           tulong ::
                                                           (tptr tuchar) ::
                                                           tulong ::
                                                           (tptr tulong) ::
                                                           nil) tint
                                                          cc_default)))
                                                        ((Etempvar _t'26 (tptr tvoid)) ::
                                                         (Etempvar _i__1 tulong) ::
                                                         (Ebinop Oadd
                                                           (Etempvar _out (tptr tuchar))
                                                           (Etempvar _t'27 tulong)
                                                           (tptr tuchar)) ::
                                                         (Ebinop Osub
                                                           (Etempvar _out_cap tulong)
                                                           (Etempvar _t'28 tulong)
                                                           tulong) ::
                                                         (Eaddrof
                                                           (Evar _written tulong)
                                                           (tptr tulong)) ::
                                                         nil))))))
                                              (Sset _err
                                                (Etempvar _t'12 tint)))
                                            (Ssequence
                                              (Ssequence
                                                (Sifthenelse (Ebinop Oeq
                                                               (Etempvar _err tint)
                                                               (Econst_int (Int.repr 0) tint)
                                                               tint)
                                                  (Ssequence
                                                    (Sset _t'23
                                                      (Evar _written tulong))
                                                    (Ssequence
                                                      (Sset _t'24
                                                        (Evar _expected_len tulong))
                                                      (Sset _t'13
                                                        (Ecast
                                                          (Ebinop One
                                                            (Etempvar _t'23 tulong)
                                                            (Etempvar _t'24 tulong)
                                                            tint) tbool))))
                                                  (Sset _t'13
                                                    (Econst_int (Int.repr 0) tint)))
                                                (Sifthenelse (Etempvar _t'13 tint)
                                                  (Sset _err
                                                    (Econst_int (Int.repr 8) tint))
                                                  Sskip))
                                              (Ssequence
                                                (Sifthenelse (Ebinop Oeq
                                                               (Etempvar _err tint)
                                                               (Econst_int (Int.repr 0) tint)
                                                               tint)
                                                  (Ssequence
                                                    (Ssequence
                                                      (Sset _t'21
                                                        (Evar _cursor tulong))
                                                      (Ssequence
                                                        (Sset _t'22
                                                          (Evar _written tulong))
                                                        (Scall (Some _t'15)
                                                          (Evar _ssz_internal_add_overflow_size 
                                                          (Tfunction
                                                            (tulong ::
                                                             tulong ::
                                                             (tptr tulong) ::
                                                             nil) tbool
                                                            cc_default))
                                                          ((Etempvar _t'21 tulong) ::
                                                           (Etempvar _t'22 tulong) ::
                                                           (Eaddrof
                                                             (Evar _cursor tulong)
                                                             (tptr tulong)) ::
                                                           nil))))
                                                    (Sset _t'14
                                                      (Ecast
                                                        (Etempvar _t'15 tbool)
                                                        tbool)))
                                                  (Sset _t'14
                                                    (Econst_int (Int.repr 0) tint)))
                                                (Sifthenelse (Etempvar _t'14 tint)
                                                  (Sset _err
                                                    (Econst_int (Int.repr 3) tint))
                                                  Sskip)))))
                                        Sskip)))))))
                          (Sset _i__1
                            (Ebinop Oadd (Etempvar _i__1 tulong)
                              (Econst_int (Int.repr 1) tint) tulong))))
                      (Ssequence
                        (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                                       (Econst_int (Int.repr 0) tint) tint)
                          (Ssequence
                            (Sset _t'18 (Evar _cursor tulong))
                            (Ssequence
                              (Sset _t'19 (Evar _total tulong))
                              (Sset _t'16
                                (Ecast
                                  (Ebinop One (Etempvar _t'18 tulong)
                                    (Etempvar _t'19 tulong) tint) tbool))))
                          (Sset _t'16 (Econst_int (Int.repr 0) tint)))
                        (Sifthenelse (Etempvar _t'16 tint)
                          (Sset _err (Econst_int (Int.repr 8) tint))
                          Sskip))))
                  Sskip))
              (Sreturn (Some (Etempvar _err tint))))))))))
|}.

Definition f_ssz_serialize_list_fixed := {|
  fn_return := tint;
  fn_callconv := cc_default;
  fn_params := ((_elements, (tptr tuchar)) :: (_element_count, tulong) ::
                (_element_limit, tulong) :: (_element_size, tulong) ::
                (_out, (tptr tuchar)) :: (_out_cap, tulong) ::
                (_out_len, (tptr tulong)) :: nil);
  fn_vars := ((_required, tulong) :: nil);
  fn_temps := ((_err, tint) :: (_t'7, tint) :: (_t'6, tbool) ::
               (_t'5, tbool) :: (_t'4, tint) :: (_t'3, tint) ::
               (_t'2, tint) :: (_t'1, tint) :: (_t'12, tulong) ::
               (_t'11, tulong) :: (_t'10, tulong) :: (_t'9, tulong) ::
               (_t'8, tulong) :: nil);
  fn_body :=
(Ssequence
  (Sassign (Evar _required tulong) (Econst_int (Int.repr 0) tuint))
  (Ssequence
    (Sset _err (Econst_int (Int.repr 0) tint))
    (Ssequence
      (Sifthenelse (Ebinop Oeq (Etempvar _element_size tulong)
                     (Econst_int (Int.repr 0) tuint) tint)
        (Sset _err (Econst_int (Int.repr 5) tint))
        (Ssequence
          (Sifthenelse (Ebinop One (Etempvar _element_limit tulong)
                         (Econst_long (Int64.repr (-1)) tulong) tint)
            (Sset _t'7
              (Ecast
                (Ebinop Ogt (Etempvar _element_count tulong)
                  (Etempvar _element_limit tulong) tint) tbool))
            (Sset _t'7 (Econst_int (Int.repr 0) tint)))
          (Sifthenelse (Etempvar _t'7 tint)
            (Sset _err (Econst_int (Int.repr 4) tint))
            (Ssequence
              (Scall (Some _t'6)
                (Evar _ssz_internal_u64_to_size (Tfunction
                                                  (tulong :: (tptr tulong) ::
                                                   nil) tbool cc_default))
                ((Etempvar _element_count tulong) ::
                 (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid)) :: nil))
              (Sifthenelse (Eunop Onotbool (Etempvar _t'6 tbool) tint)
                (Sset _err (Econst_int (Int.repr 3) tint))
                (Ssequence
                  (Scall (Some _t'5)
                    (Evar _ssz_internal_mul_overflow_size (Tfunction
                                                            (tulong ::
                                                             tulong ::
                                                             (tptr tulong) ::
                                                             nil) tbool
                                                            cc_default))
                    ((Ecast (Etempvar _element_count tulong) tulong) ::
                     (Etempvar _element_size tulong) ::
                     (Eaddrof (Evar _required tulong) (tptr tulong)) :: nil))
                  (Sifthenelse (Etempvar _t'5 tbool)
                    (Sset _err (Econst_int (Int.repr 3) tint))
                    (Ssequence
                      (Sset _t'8 (Evar _required tulong))
                      (Sifthenelse (Ebinop Ogt (Etempvar _t'8 tulong)
                                     (Econst_int (Int.repr (-1)) tuint) tint)
                        (Sset _err (Econst_int (Int.repr 3) tint))
                        (Ssequence
                          (Ssequence
                            (Sset _t'12 (Evar _required tulong))
                            (Sifthenelse (Ebinop One (Etempvar _t'12 tulong)
                                           (Econst_int (Int.repr 0) tuint)
                                           tint)
                              (Sset _t'4
                                (Ecast
                                  (Ebinop Oeq
                                    (Etempvar _elements (tptr tuchar))
                                    (Ecast (Econst_int (Int.repr 0) tint)
                                      (tptr tvoid)) tint) tbool))
                              (Sset _t'4 (Econst_int (Int.repr 0) tint))))
                          (Sifthenelse (Etempvar _t'4 tint)
                            (Sset _err (Econst_int (Int.repr 1) tint))
                            (Ssequence
                              (Ssequence
                                (Ssequence
                                  (Sset _t'11 (Evar _required tulong))
                                  (Scall (Some _t'1)
                                    (Evar _ssz_internal_prepare_output 
                                    (Tfunction
                                      (tulong :: (tptr tuchar) :: tulong ::
                                       (tptr tulong) :: nil) tint cc_default))
                                    ((Etempvar _t'11 tulong) ::
                                     (Etempvar _out (tptr tuchar)) ::
                                     (Etempvar _out_cap tulong) ::
                                     (Etempvar _out_len (tptr tulong)) ::
                                     nil)))
                                (Sset _err (Etempvar _t'1 tint)))
                              (Ssequence
                                (Ssequence
                                  (Sifthenelse (Ebinop Oeq
                                                 (Etempvar _err tint)
                                                 (Econst_int (Int.repr 0) tint)
                                                 tint)
                                    (Sset _t'2
                                      (Ecast
                                        (Ebinop One
                                          (Etempvar _out (tptr tuchar))
                                          (Ecast
                                            (Econst_int (Int.repr 0) tint)
                                            (tptr tvoid)) tint) tbool))
                                    (Sset _t'2
                                      (Econst_int (Int.repr 0) tint)))
                                  (Sifthenelse (Etempvar _t'2 tint)
                                    (Ssequence
                                      (Sset _t'10 (Evar _required tulong))
                                      (Sset _t'3
                                        (Ecast
                                          (Ebinop One (Etempvar _t'10 tulong)
                                            (Econst_int (Int.repr 0) tuint)
                                            tint) tbool)))
                                    (Sset _t'3
                                      (Econst_int (Int.repr 0) tint))))
                                (Sifthenelse (Etempvar _t'3 tint)
                                  (Ssequence
                                    (Sset _t'9 (Evar _required tulong))
                                    (Scall None
                                      (Evar _memcpy (Tfunction
                                                      ((tptr tvoid) ::
                                                       (tptr tvoid) ::
                                                       tulong :: nil)
                                                      (tptr tvoid)
                                                      cc_default))
                                      ((Etempvar _out (tptr tuchar)) ::
                                       (Etempvar _elements (tptr tuchar)) ::
                                       (Etempvar _t'9 tulong) :: nil)))
                                  Sskip))))))))))))))
      (Sreturn (Some (Etempvar _err tint))))))
|}.

Definition f_ssz_serialize_list_variable := {|
  fn_return := tint;
  fn_callconv := cc_default;
  fn_params := ((_element_count, tulong) :: (_element_limit, tulong) ::
                (_codec, (tptr (Tstruct __435 noattr))) ::
                (_out, (tptr tuchar)) :: (_out_cap, tulong) ::
                (_out_len, (tptr tulong)) :: nil);
  fn_vars := ((_fixed_region, tulong) :: (_cursor, tulong) ::
              (_written, tulong) :: (_total, tulong) ::
              (_encoded_len, tulong) :: nil);
  fn_temps := ((_err, tint) :: (_i, tulong) :: (_i__1, tulong) ::
               (_t'15, tint) :: (_t'14, tint) :: (_t'13, tbool) ::
               (_t'12, tbool) :: (_t'11, tint) :: (_t'10, tint) ::
               (_t'9, tbool) :: (_t'8, tint) :: (_t'7, tint) ::
               (_t'6, tint) :: (_t'5, tint) :: (_t'4, tbool) ::
               (_t'3, tint) :: (_t'2, tint) :: (_t'1, tint) ::
               (_t'37,
                (tptr (Tfunction
                        ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong ::
                         (tptr tulong) :: nil) tint cc_default))) ::
               (_t'36, tulong) :: (_t'35, tulong) :: (_t'34, tulong) ::
               (_t'33, tulong) :: (_t'32, (tptr tvoid)) ::
               (_t'31,
                (tptr (Tfunction
                        ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong ::
                         (tptr tulong) :: nil) tint cc_default))) ::
               (_t'30, tulong) :: (_t'29, tulong) :: (_t'28, tulong) ::
               (_t'27, tulong) :: (_t'26, tulong) :: (_t'25, tulong) ::
               (_t'24, tulong) :: (_t'23, tulong) :: (_t'22, (tptr tvoid)) ::
               (_t'21,
                (tptr (Tfunction
                        ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong ::
                         (tptr tulong) :: nil) tint cc_default))) ::
               (_t'20, tulong) :: (_t'19, tulong) :: (_t'18, tulong) ::
               (_t'17, tulong) :: (_t'16, tulong) :: nil);
  fn_body :=
(Ssequence
  (Sassign (Evar _fixed_region tulong) (Econst_int (Int.repr 0) tuint))
  (Ssequence
    (Sset _err (Econst_int (Int.repr 0) tint))
    (Ssequence
      (Ssequence
        (Sifthenelse (Ebinop One (Etempvar _element_limit tulong)
                       (Econst_long (Int64.repr (-1)) tulong) tint)
          (Sset _t'15
            (Ecast
              (Ebinop Ogt (Etempvar _element_count tulong)
                (Etempvar _element_limit tulong) tint) tbool))
          (Sset _t'15 (Econst_int (Int.repr 0) tint)))
        (Sifthenelse (Etempvar _t'15 tint)
          (Sset _err (Econst_int (Int.repr 4) tint))
          (Ssequence
            (Sifthenelse (Ebinop Oeq
                           (Etempvar _codec (tptr (Tstruct __435 noattr)))
                           (Ecast (Econst_int (Int.repr 0) tint)
                             (tptr tvoid)) tint)
              (Sset _t'14 (Econst_int (Int.repr 1) tint))
              (Ssequence
                (Sset _t'37
                  (Efield
                    (Ederef (Etempvar _codec (tptr (Tstruct __435 noattr)))
                      (Tstruct __435 noattr)) _write
                    (tptr (Tfunction
                            ((tptr tvoid) :: tulong :: (tptr tuchar) ::
                             tulong :: (tptr tulong) :: nil) tint cc_default))))
                (Sset _t'14
                  (Ecast
                    (Ebinop Oeq
                      (Etempvar _t'37 (tptr (Tfunction
                                              ((tptr tvoid) :: tulong ::
                                               (tptr tuchar) :: tulong ::
                                               (tptr tulong) :: nil) tint
                                              cc_default)))
                      (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid))
                      tint) tbool))))
            (Sifthenelse (Etempvar _t'14 tint)
              (Sset _err (Econst_int (Int.repr 1) tint))
              (Ssequence
                (Scall (Some _t'13)
                  (Evar _ssz_internal_u64_to_size (Tfunction
                                                    (tulong ::
                                                     (tptr tulong) :: nil)
                                                    tbool cc_default))
                  ((Etempvar _element_count tulong) ::
                   (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid)) ::
                   nil))
                (Sifthenelse (Eunop Onotbool (Etempvar _t'13 tbool) tint)
                  (Sset _err (Econst_int (Int.repr 3) tint))
                  (Ssequence
                    (Scall (Some _t'12)
                      (Evar _ssz_internal_mul_overflow_size (Tfunction
                                                              (tulong ::
                                                               tulong ::
                                                               (tptr tulong) ::
                                                               nil) tbool
                                                              cc_default))
                      ((Ecast (Etempvar _element_count tulong) tulong) ::
                       (Econst_int (Int.repr 4) tuint) ::
                       (Eaddrof (Evar _fixed_region tulong) (tptr tulong)) ::
                       nil))
                    (Sifthenelse (Etempvar _t'12 tbool)
                      (Sset _err (Econst_int (Int.repr 3) tint))
                      (Ssequence
                        (Sset _t'16 (Evar _fixed_region tulong))
                        (Sifthenelse (Ebinop Ogt (Etempvar _t'16 tulong)
                                       (Econst_int (Int.repr (-1)) tuint)
                                       tint)
                          (Sset _err (Econst_int (Int.repr 3) tint))
                          (Sifthenelse (Ebinop One
                                         (Etempvar _out (tptr tuchar))
                                         (Ecast
                                           (Econst_int (Int.repr 0) tint)
                                           (tptr tvoid)) tint)
                            (Sifthenelse (Ebinop Oeq
                                           (Etempvar _out_len (tptr tulong))
                                           (Ecast
                                             (Econst_int (Int.repr 0) tint)
                                             (tptr tvoid)) tint)
                              (Sset _err (Econst_int (Int.repr 1) tint))
                              (Ssequence
                                (Sset _t'24 (Evar _fixed_region tulong))
                                (Sifthenelse (Ebinop Olt
                                               (Etempvar _out_cap tulong)
                                               (Etempvar _t'24 tulong) tint)
                                  (Sset _err (Econst_int (Int.repr 2) tint))
                                  (Ssequence
                                    (Ssequence
                                      (Sset _t'36
                                        (Evar _fixed_region tulong))
                                      (Sassign (Evar _cursor tulong)
                                        (Etempvar _t'36 tulong)))
                                    (Ssequence
                                      (Ssequence
                                        (Sset _i
                                          (Ecast
                                            (Econst_int (Int.repr 0) tuint)
                                            tulong))
                                        (Sloop
                                          (Ssequence
                                            (Ssequence
                                              (Sifthenelse (Ebinop Olt
                                                             (Etempvar _i tulong)
                                                             (Etempvar _element_count tulong)
                                                             tint)
                                                (Sset _t'1
                                                  (Ecast
                                                    (Ebinop Oeq
                                                      (Etempvar _err tint)
                                                      (Econst_int (Int.repr 0) tint)
                                                      tint) tbool))
                                                (Sset _t'1
                                                  (Econst_int (Int.repr 0) tint)))
                                              (Sifthenelse (Etempvar _t'1 tint)
                                                Sskip
                                                Sbreak))
                                            (Ssequence
                                              (Sset _t'27
                                                (Evar _cursor tulong))
                                              (Sifthenelse (Ebinop Ogt
                                                             (Etempvar _t'27 tulong)
                                                             (Econst_int (Int.repr (-1)) tuint)
                                                             tint)
                                                (Sset _err
                                                  (Econst_int (Int.repr 3) tint))
                                                (Ssequence
                                                  (Ssequence
                                                    (Sset _t'35
                                                      (Evar _cursor tulong))
                                                    (Scall None
                                                      (Evar _ssz_internal_write_u32_le 
                                                      (Tfunction
                                                        ((tptr tuchar) ::
                                                         tuint :: nil) tvoid
                                                        cc_default))
                                                      ((Ebinop Oadd
                                                         (Etempvar _out (tptr tuchar))
                                                         (Ebinop Omul
                                                           (Ecast
                                                             (Etempvar _i tulong)
                                                             tulong)
                                                           (Econst_int (Int.repr 4) tuint)
                                                           tulong)
                                                         (tptr tuchar)) ::
                                                       (Ecast
                                                         (Etempvar _t'35 tulong)
                                                         tuint) :: nil)))
                                                  (Ssequence
                                                    (Sset _t'28
                                                      (Evar _cursor tulong))
                                                    (Sifthenelse (Ebinop Ogt
                                                                   (Etempvar _t'28 tulong)
                                                                   (Etempvar _out_cap tulong)
                                                                   tint)
                                                      (Sset _err
                                                        (Econst_int (Int.repr 2) tint))
                                                      (Ssequence
                                                        (Sassign
                                                          (Evar _written tulong)
                                                          (Econst_int (Int.repr 0) tuint))
                                                        (Ssequence
                                                          (Ssequence
                                                            (Ssequence
                                                              (Sset _t'31
                                                                (Efield
                                                                  (Ederef
                                                                    (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                                                    (Tstruct __435 noattr))
                                                                  _write
                                                                  (tptr 
                                                                  (Tfunction
                                                                    ((tptr tvoid) ::
                                                                    tulong ::
                                                                    (tptr tuchar) ::
                                                                    tulong ::
                                                                    (tptr tulong) ::
                                                                    nil) tint
                                                                    cc_default))))
                                                              (Ssequence
                                                                (Sset _t'32
                                                                  (Efield
                                                                    (Ederef
                                                                    (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                                                    (Tstruct __435 noattr))
                                                                    _ctx
                                                                    (tptr tvoid)))
                                                                (Ssequence
                                                                  (Sset _t'33
                                                                    (Evar _cursor tulong))
                                                                  (Ssequence
                                                                    (Sset _t'34
                                                                    (Evar _cursor tulong))
                                                                    (Scall (Some _t'2)
                                                                    (Etempvar _t'31 (tptr 
                                                                    (Tfunction
                                                                    ((tptr tvoid) ::
                                                                    tulong ::
                                                                    (tptr tuchar) ::
                                                                    tulong ::
                                                                    (tptr tulong) ::
                                                                    nil) tint
                                                                    cc_default)))
                                                                    ((Etempvar _t'32 (tptr tvoid)) ::
                                                                    (Etempvar _i tulong) ::
                                                                    (Ebinop Oadd
                                                                    (Etempvar _out (tptr tuchar))
                                                                    (Etempvar _t'33 tulong)
                                                                    (tptr tuchar)) ::
                                                                    (Ebinop Osub
                                                                    (Etempvar _out_cap tulong)
                                                                    (Etempvar _t'34 tulong)
                                                                    tulong) ::
                                                                    (Eaddrof
                                                                    (Evar _written tulong)
                                                                    (tptr tulong)) ::
                                                                    nil))))))
                                                            (Sset _err
                                                              (Etempvar _t'2 tint)))
                                                          (Ssequence
                                                            (Sifthenelse 
                                                              (Ebinop Oeq
                                                                (Etempvar _err tint)
                                                                (Econst_int (Int.repr 0) tint)
                                                                tint)
                                                              (Ssequence
                                                                (Ssequence
                                                                  (Sset _t'29
                                                                    (Evar _cursor tulong))
                                                                  (Ssequence
                                                                    (Sset _t'30
                                                                    (Evar _written tulong))
                                                                    (Scall (Some _t'4)
                                                                    (Evar _ssz_internal_add_overflow_size 
                                                                    (Tfunction
                                                                    (tulong ::
                                                                    tulong ::
                                                                    (tptr tulong) ::
                                                                    nil)
                                                                    tbool
                                                                    cc_default))
                                                                    ((Etempvar _t'29 tulong) ::
                                                                    (Etempvar _t'30 tulong) ::
                                                                    (Eaddrof
                                                                    (Evar _cursor tulong)
                                                                    (tptr tulong)) ::
                                                                    nil))))
                                                                (Sset _t'3
                                                                  (Ecast
                                                                    (Etempvar _t'4 tbool)
                                                                    tbool)))
                                                              (Sset _t'3
                                                                (Econst_int (Int.repr 0) tint)))
                                                            (Sifthenelse (Etempvar _t'3 tint)
                                                              (Sset _err
                                                                (Econst_int (Int.repr 3) tint))
                                                              Sskip))))))))))
                                          (Sset _i
                                            (Ebinop Oadd (Etempvar _i tulong)
                                              (Econst_int (Int.repr 1) tint)
                                              tulong))))
                                      (Ssequence
                                        (Ssequence
                                          (Sifthenelse (Ebinop Oeq
                                                         (Etempvar _err tint)
                                                         (Econst_int (Int.repr 0) tint)
                                                         tint)
                                            (Ssequence
                                              (Sset _t'26
                                                (Evar _cursor tulong))
                                              (Sset _t'5
                                                (Ecast
                                                  (Ebinop Ogt
                                                    (Etempvar _t'26 tulong)
                                                    (Econst_int (Int.repr (-1)) tuint)
                                                    tint) tbool)))
                                            (Sset _t'5
                                              (Econst_int (Int.repr 0) tint)))
                                          (Sifthenelse (Etempvar _t'5 tint)
                                            (Sset _err
                                              (Econst_int (Int.repr 3) tint))
                                            Sskip))
                                        (Sifthenelse (Ebinop Oeq
                                                       (Etempvar _err tint)
                                                       (Econst_int (Int.repr 0) tint)
                                                       tint)
                                          (Ssequence
                                            (Sset _t'25
                                              (Evar _cursor tulong))
                                            (Sassign
                                              (Ederef
                                                (Etempvar _out_len (tptr tulong))
                                                tulong)
                                              (Etempvar _t'25 tulong)))
                                          Sskip)))))))
                            (Ssequence
                              (Ssequence
                                (Sset _t'23 (Evar _fixed_region tulong))
                                (Sassign (Evar _total tulong)
                                  (Etempvar _t'23 tulong)))
                              (Ssequence
                                (Ssequence
                                  (Sset _i__1
                                    (Ecast (Econst_int (Int.repr 0) tuint)
                                      tulong))
                                  (Sloop
                                    (Ssequence
                                      (Ssequence
                                        (Sifthenelse (Ebinop Olt
                                                       (Etempvar _i__1 tulong)
                                                       (Etempvar _element_count tulong)
                                                       tint)
                                          (Sset _t'6
                                            (Ecast
                                              (Ebinop Oeq
                                                (Etempvar _err tint)
                                                (Econst_int (Int.repr 0) tint)
                                                tint) tbool))
                                          (Sset _t'6
                                            (Econst_int (Int.repr 0) tint)))
                                        (Sifthenelse (Etempvar _t'6 tint)
                                          Sskip
                                          Sbreak))
                                      (Ssequence
                                        (Sassign (Evar _encoded_len tulong)
                                          (Econst_int (Int.repr 0) tuint))
                                        (Ssequence
                                          (Ssequence
                                            (Ssequence
                                              (Sset _t'21
                                                (Efield
                                                  (Ederef
                                                    (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                                    (Tstruct __435 noattr))
                                                  _write
                                                  (tptr (Tfunction
                                                          ((tptr tvoid) ::
                                                           tulong ::
                                                           (tptr tuchar) ::
                                                           tulong ::
                                                           (tptr tulong) ::
                                                           nil) tint
                                                          cc_default))))
                                              (Ssequence
                                                (Sset _t'22
                                                  (Efield
                                                    (Ederef
                                                      (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                                      (Tstruct __435 noattr))
                                                    _ctx (tptr tvoid)))
                                                (Scall (Some _t'7)
                                                  (Etempvar _t'21 (tptr 
                                                  (Tfunction
                                                    ((tptr tvoid) ::
                                                     tulong ::
                                                     (tptr tuchar) ::
                                                     tulong ::
                                                     (tptr tulong) :: nil)
                                                    tint cc_default)))
                                                  ((Etempvar _t'22 (tptr tvoid)) ::
                                                   (Etempvar _i__1 tulong) ::
                                                   (Ecast
                                                     (Econst_int (Int.repr 0) tint)
                                                     (tptr tvoid)) ::
                                                   (Econst_int (Int.repr 0) tuint) ::
                                                   (Eaddrof
                                                     (Evar _encoded_len tulong)
                                                     (tptr tulong)) :: nil))))
                                            (Sset _err (Etempvar _t'7 tint)))
                                          (Ssequence
                                            (Sifthenelse (Ebinop Oeq
                                                           (Etempvar _err tint)
                                                           (Econst_int (Int.repr 0) tint)
                                                           tint)
                                              (Ssequence
                                                (Ssequence
                                                  (Sset _t'19
                                                    (Evar _total tulong))
                                                  (Ssequence
                                                    (Sset _t'20
                                                      (Evar _encoded_len tulong))
                                                    (Scall (Some _t'9)
                                                      (Evar _ssz_internal_add_overflow_size 
                                                      (Tfunction
                                                        (tulong :: tulong ::
                                                         (tptr tulong) ::
                                                         nil) tbool
                                                        cc_default))
                                                      ((Etempvar _t'19 tulong) ::
                                                       (Etempvar _t'20 tulong) ::
                                                       (Eaddrof
                                                         (Evar _total tulong)
                                                         (tptr tulong)) ::
                                                       nil))))
                                                (Sset _t'8
                                                  (Ecast
                                                    (Etempvar _t'9 tbool)
                                                    tbool)))
                                              (Sset _t'8
                                                (Econst_int (Int.repr 0) tint)))
                                            (Sifthenelse (Etempvar _t'8 tint)
                                              (Sset _err
                                                (Econst_int (Int.repr 3) tint))
                                              Sskip)))))
                                    (Sset _i__1
                                      (Ebinop Oadd (Etempvar _i__1 tulong)
                                        (Econst_int (Int.repr 1) tint)
                                        tulong))))
                                (Ssequence
                                  (Ssequence
                                    (Sifthenelse (Ebinop Oeq
                                                   (Etempvar _err tint)
                                                   (Econst_int (Int.repr 0) tint)
                                                   tint)
                                      (Ssequence
                                        (Sset _t'18 (Evar _total tulong))
                                        (Sset _t'10
                                          (Ecast
                                            (Ebinop Ogt
                                              (Etempvar _t'18 tulong)
                                              (Econst_int (Int.repr (-1)) tuint)
                                              tint) tbool)))
                                      (Sset _t'10
                                        (Econst_int (Int.repr 0) tint)))
                                    (Sifthenelse (Etempvar _t'10 tint)
                                      (Sset _err
                                        (Econst_int (Int.repr 3) tint))
                                      Sskip))
                                  (Sifthenelse (Ebinop Oeq
                                                 (Etempvar _err tint)
                                                 (Econst_int (Int.repr 0) tint)
                                                 tint)
                                    (Ssequence
                                      (Ssequence
                                        (Sset _t'17 (Evar _total tulong))
                                        (Scall (Some _t'11)
                                          (Evar _ssz_internal_prepare_output 
                                          (Tfunction
                                            (tulong :: (tptr tuchar) ::
                                             tulong :: (tptr tulong) :: nil)
                                            tint cc_default))
                                          ((Etempvar _t'17 tulong) ::
                                           (Etempvar _out (tptr tuchar)) ::
                                           (Etempvar _out_cap tulong) ::
                                           (Etempvar _out_len (tptr tulong)) ::
                                           nil)))
                                      (Sset _err (Etempvar _t'11 tint)))
                                    Sskip)))))))))))))))
      (Sreturn (Some (Etempvar _err tint))))))
|}.

Definition f_ssz_serialize_container := {|
  fn_return := tint;
  fn_callconv := cc_default;
  fn_params := ((_schema, (tptr (Tstruct __449 noattr))) ::
                (_codec, (tptr (Tstruct __435 noattr))) ::
                (_out, (tptr tuchar)) :: (_out_cap, tulong) ::
                (_out_len, (tptr tulong)) :: nil);
  fn_vars := ((_fixed_region, tulong) :: (_variable_cursor, tulong) ::
              (_written, tulong) :: (_next_fixed_cursor, tulong) ::
              (_next_fixed_cursor__1, tulong) :: (_total, tulong) ::
              (_encoded_len, tulong) :: nil);
  fn_temps := ((_field_fixed_sizes, (tptr tulong)) ::
               (_field_count, tuint) :: (_err, tint) :: (_i, tuint) ::
               (_fixed_size, tulong) :: (_contribution, tulong) ::
               (_fixed_cursor, tulong) :: (_i__1, tuint) ::
               (_fixed_size__1, tulong) :: (_i__2, tuint) :: (_t'24, tint) ::
               (_t'23, tint) :: (_t'22, tint) :: (_t'21, tbool) ::
               (_t'20, tint) :: (_t'19, tint) :: (_t'18, tint) ::
               (_t'17, tint) :: (_t'16, tint) :: (_t'15, tbool) ::
               (_t'14, tint) :: (_t'13, tint) :: (_t'12, tint) ::
               (_t'11, tbool) :: (_t'10, tint) :: (_t'9, tbool) ::
               (_t'8, tint) :: (_t'7, tint) :: (_t'6, tint) ::
               (_t'5, tint) :: (_t'4, tint) :: (_t'3, tint) ::
               (_t'2, tbool) :: (_t'1, tulong) :: (_t'62, (tptr tulong)) ::
               (_t'61, tuint) ::
               (_t'60,
                (tptr (Tfunction
                        ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong ::
                         (tptr tulong) :: nil) tint cc_default))) ::
               (_t'59, tulong) :: (_t'58, tulong) :: (_t'57, tulong) ::
               (_t'56, tulong) :: (_t'55, tulong) :: (_t'54, tulong) ::
               (_t'53, tulong) :: (_t'52, tulong) :: (_t'51, (tptr tvoid)) ::
               (_t'50,
                (tptr (Tfunction
                        ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong ::
                         (tptr tulong) :: nil) tint cc_default))) ::
               (_t'49, tulong) :: (_t'48, tulong) :: (_t'47, tulong) ::
               (_t'46, tulong) :: (_t'45, tulong) :: (_t'44, tulong) ::
               (_t'43, tulong) :: (_t'42, (tptr tvoid)) ::
               (_t'41,
                (tptr (Tfunction
                        ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong ::
                         (tptr tulong) :: nil) tint cc_default))) ::
               (_t'40, tulong) :: (_t'39, tulong) :: (_t'38, tulong) ::
               (_t'37, tulong) :: (_t'36, tulong) :: (_t'35, tulong) ::
               (_t'34, tulong) :: (_t'33, (tptr tvoid)) ::
               (_t'32,
                (tptr (Tfunction
                        ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong ::
                         (tptr tulong) :: nil) tint cc_default))) ::
               (_t'31, tulong) :: (_t'30, tulong) :: (_t'29, tulong) ::
               (_t'28, tulong) :: (_t'27, tulong) :: (_t'26, tulong) ::
               (_t'25, tulong) :: nil);
  fn_body :=
(Ssequence
  (Sset _field_fixed_sizes
    (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid)))
  (Ssequence
    (Sset _field_count (Econst_int (Int.repr 0) tuint))
    (Ssequence
      (Sassign (Evar _fixed_region tulong) (Econst_int (Int.repr 0) tuint))
      (Ssequence
        (Sset _err (Econst_int (Int.repr 0) tint))
        (Ssequence
          (Ssequence
            (Ssequence
              (Sifthenelse (Ebinop Oeq
                             (Etempvar _schema (tptr (Tstruct __449 noattr)))
                             (Ecast (Econst_int (Int.repr 0) tint)
                               (tptr tvoid)) tint)
                (Sset _t'4 (Econst_int (Int.repr 1) tint))
                (Ssequence
                  (Sset _t'62
                    (Efield
                      (Ederef
                        (Etempvar _schema (tptr (Tstruct __449 noattr)))
                        (Tstruct __449 noattr)) _field_fixed_sizes
                      (tptr tulong)))
                  (Sset _t'4
                    (Ecast
                      (Ebinop Oeq (Etempvar _t'62 (tptr tulong))
                        (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid))
                        tint) tbool))))
              (Sifthenelse (Etempvar _t'4 tint)
                (Sset _t'5 (Econst_int (Int.repr 1) tint))
                (Ssequence
                  (Sset _t'61
                    (Efield
                      (Ederef
                        (Etempvar _schema (tptr (Tstruct __449 noattr)))
                        (Tstruct __449 noattr)) _field_count tuint))
                  (Sset _t'5
                    (Ecast
                      (Ebinop Oeq (Etempvar _t'61 tuint)
                        (Econst_int (Int.repr 0) tuint) tint) tbool)))))
            (Sifthenelse (Etempvar _t'5 tint)
              (Sset _err (Econst_int (Int.repr 5) tint))
              (Ssequence
                (Sifthenelse (Ebinop Oeq
                               (Etempvar _codec (tptr (Tstruct __435 noattr)))
                               (Ecast (Econst_int (Int.repr 0) tint)
                                 (tptr tvoid)) tint)
                  (Sset _t'3 (Econst_int (Int.repr 1) tint))
                  (Ssequence
                    (Sset _t'60
                      (Efield
                        (Ederef
                          (Etempvar _codec (tptr (Tstruct __435 noattr)))
                          (Tstruct __435 noattr)) _write
                        (tptr (Tfunction
                                ((tptr tvoid) :: tulong :: (tptr tuchar) ::
                                 tulong :: (tptr tulong) :: nil) tint
                                cc_default))))
                    (Sset _t'3
                      (Ecast
                        (Ebinop Oeq
                          (Etempvar _t'60 (tptr (Tfunction
                                                  ((tptr tvoid) :: tulong ::
                                                   (tptr tuchar) :: tulong ::
                                                   (tptr tulong) :: nil) tint
                                                  cc_default)))
                          (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid))
                          tint) tbool))))
                (Sifthenelse (Etempvar _t'3 tint)
                  (Sset _err (Econst_int (Int.repr 1) tint))
                  (Ssequence
                    (Sset _field_fixed_sizes
                      (Efield
                        (Ederef
                          (Etempvar _schema (tptr (Tstruct __449 noattr)))
                          (Tstruct __449 noattr)) _field_fixed_sizes
                        (tptr tulong)))
                    (Ssequence
                      (Sset _field_count
                        (Efield
                          (Ederef
                            (Etempvar _schema (tptr (Tstruct __449 noattr)))
                            (Tstruct __449 noattr)) _field_count tuint))
                      (Ssequence
                        (Sset _i (Econst_int (Int.repr 0) tuint))
                        (Sloop
                          (Ssequence
                            (Sifthenelse (Ebinop Olt (Etempvar _i tuint)
                                           (Etempvar _field_count tuint)
                                           tint)
                              Sskip
                              Sbreak)
                            (Ssequence
                              (Sset _fixed_size
                                (Ederef
                                  (Ebinop Oadd
                                    (Etempvar _field_fixed_sizes (tptr tulong))
                                    (Etempvar _i tuint) (tptr tulong))
                                  tulong))
                              (Ssequence
                                (Ssequence
                                  (Sifthenelse (Ebinop Oeq
                                                 (Etempvar _fixed_size tulong)
                                                 (Econst_int (Int.repr 0) tuint)
                                                 tint)
                                    (Sset _t'1
                                      (Ecast (Econst_int (Int.repr 4) tuint)
                                        tulong))
                                    (Sset _t'1
                                      (Ecast (Etempvar _fixed_size tulong)
                                        tulong)))
                                  (Sset _contribution (Etempvar _t'1 tulong)))
                                (Ssequence
                                  (Ssequence
                                    (Sset _t'59 (Evar _fixed_region tulong))
                                    (Scall (Some _t'2)
                                      (Evar _ssz_internal_add_overflow_size 
                                      (Tfunction
                                        (tulong :: tulong :: (tptr tulong) ::
                                         nil) tbool cc_default))
                                      ((Etempvar _t'59 tulong) ::
                                       (Etempvar _contribution tulong) ::
                                       (Eaddrof (Evar _fixed_region tulong)
                                         (tptr tulong)) :: nil)))
                                  (Sifthenelse (Etempvar _t'2 tbool)
                                    (Ssequence
                                      (Sset _err
                                        (Econst_int (Int.repr 3) tint))
                                      Sbreak)
                                    Sskip)))))
                          (Sset _i
                            (Ebinop Oadd (Etempvar _i tuint)
                              (Econst_int (Int.repr 1) tint) tuint))))))))))
          (Ssequence
            (Ssequence
              (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                             (Econst_int (Int.repr 0) tint) tint)
                (Sset _t'18
                  (Ecast
                    (Ebinop One (Etempvar _out (tptr tuchar))
                      (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid))
                      tint) tbool))
                (Sset _t'18 (Econst_int (Int.repr 0) tint)))
              (Sifthenelse (Etempvar _t'18 tint)
                (Sifthenelse (Ebinop Oeq (Etempvar _out_len (tptr tulong))
                               (Ecast (Econst_int (Int.repr 0) tint)
                                 (tptr tvoid)) tint)
                  (Sset _err (Econst_int (Int.repr 1) tint))
                  (Ssequence
                    (Sset _t'35 (Evar _fixed_region tulong))
                    (Sifthenelse (Ebinop Ogt (Etempvar _t'35 tulong)
                                   (Econst_int (Int.repr (-1)) tuint) tint)
                      (Sset _err (Econst_int (Int.repr 3) tint))
                      (Ssequence
                        (Sset _t'36 (Evar _fixed_region tulong))
                        (Sifthenelse (Ebinop Olt (Etempvar _out_cap tulong)
                                       (Etempvar _t'36 tulong) tint)
                          (Sset _err (Econst_int (Int.repr 2) tint))
                          (Ssequence
                            (Sset _fixed_cursor
                              (Ecast (Econst_int (Int.repr 0) tuint) tulong))
                            (Ssequence
                              (Ssequence
                                (Sset _t'58 (Evar _fixed_region tulong))
                                (Sassign (Evar _variable_cursor tulong)
                                  (Etempvar _t'58 tulong)))
                              (Ssequence
                                (Ssequence
                                  (Sset _i__1
                                    (Econst_int (Int.repr 0) tuint))
                                  (Sloop
                                    (Ssequence
                                      (Ssequence
                                        (Sifthenelse (Ebinop Olt
                                                       (Etempvar _i__1 tuint)
                                                       (Etempvar _field_count tuint)
                                                       tint)
                                          (Sset _t'6
                                            (Ecast
                                              (Ebinop Oeq
                                                (Etempvar _err tint)
                                                (Econst_int (Int.repr 0) tint)
                                                tint) tbool))
                                          (Sset _t'6
                                            (Econst_int (Int.repr 0) tint)))
                                        (Sifthenelse (Etempvar _t'6 tint)
                                          Sskip
                                          Sbreak))
                                      (Ssequence
                                        (Sset _fixed_size__1
                                          (Ederef
                                            (Ebinop Oadd
                                              (Etempvar _field_fixed_sizes (tptr tulong))
                                              (Etempvar _i__1 tuint)
                                              (tptr tulong)) tulong))
                                        (Ssequence
                                          (Sassign (Evar _written tulong)
                                            (Econst_int (Int.repr 0) tuint))
                                          (Sifthenelse (Ebinop Oeq
                                                         (Etempvar _fixed_size__1 tulong)
                                                         (Econst_int (Int.repr 0) tuint)
                                                         tint)
                                            (Ssequence
                                              (Sassign
                                                (Evar _next_fixed_cursor tulong)
                                                (Econst_int (Int.repr 0) tuint))
                                              (Ssequence
                                                (Scall (Some _t'11)
                                                  (Evar _ssz_internal_add_overflow_size 
                                                  (Tfunction
                                                    (tulong :: tulong ::
                                                     (tptr tulong) :: nil)
                                                    tbool cc_default))
                                                  ((Etempvar _fixed_cursor tulong) ::
                                                   (Econst_int (Int.repr 4) tuint) ::
                                                   (Eaddrof
                                                     (Evar _next_fixed_cursor tulong)
                                                     (tptr tulong)) :: nil))
                                                (Sifthenelse (Etempvar _t'11 tbool)
                                                  (Sset _err
                                                    (Econst_int (Int.repr 3) tint))
                                                  (Ssequence
                                                    (Ssequence
                                                      (Sset _t'55
                                                        (Evar _next_fixed_cursor tulong))
                                                      (Ssequence
                                                        (Sset _t'56
                                                          (Evar _fixed_region tulong))
                                                        (Sifthenelse 
                                                          (Ebinop Ogt
                                                            (Etempvar _t'55 tulong)
                                                            (Etempvar _t'56 tulong)
                                                            tint)
                                                          (Sset _t'10
                                                            (Econst_int (Int.repr 1) tint))
                                                          (Ssequence
                                                            (Sset _t'57
                                                              (Evar _next_fixed_cursor tulong))
                                                            (Sset _t'10
                                                              (Ecast
                                                                (Ebinop Ogt
                                                                  (Etempvar _t'57 tulong)
                                                                  (Etempvar _out_cap tulong)
                                                                  tint)
                                                                tbool))))))
                                                    (Sifthenelse (Etempvar _t'10 tint)
                                                      (Sset _err
                                                        (Econst_int (Int.repr 8) tint))
                                                      (Ssequence
                                                        (Sset _t'46
                                                          (Evar _variable_cursor tulong))
                                                        (Sifthenelse 
                                                          (Ebinop Ogt
                                                            (Etempvar _t'46 tulong)
                                                            (Econst_int (Int.repr (-1)) tuint)
                                                            tint)
                                                          (Sset _err
                                                            (Econst_int (Int.repr 3) tint))
                                                          (Ssequence
                                                            (Sset _t'47
                                                              (Evar _variable_cursor tulong))
                                                            (Sifthenelse 
                                                              (Ebinop Ogt
                                                                (Etempvar _t'47 tulong)
                                                                (Etempvar _out_cap tulong)
                                                                tint)
                                                              (Sset _err
                                                                (Econst_int (Int.repr 2) tint))
                                                              (Ssequence
                                                                (Ssequence
                                                                  (Sset _t'54
                                                                    (Evar _variable_cursor tulong))
                                                                  (Scall None
                                                                    (Evar _ssz_internal_write_u32_le 
                                                                    (Tfunction
                                                                    ((tptr tuchar) ::
                                                                    tuint ::
                                                                    nil)
                                                                    tvoid
                                                                    cc_default))
                                                                    ((Ebinop Oadd
                                                                    (Etempvar _out (tptr tuchar))
                                                                    (Etempvar _fixed_cursor tulong)
                                                                    (tptr tuchar)) ::
                                                                    (Ecast
                                                                    (Etempvar _t'54 tulong)
                                                                    tuint) ::
                                                                    nil)))
                                                                (Ssequence
                                                                  (Sset _fixed_cursor
                                                                    (Evar _next_fixed_cursor tulong))
                                                                  (Ssequence
                                                                    (Ssequence
                                                                    (Ssequence
                                                                    (Sset _t'50
                                                                    (Efield
                                                                    (Ederef
                                                                    (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                                                    (Tstruct __435 noattr))
                                                                    _write
                                                                    (tptr 
                                                                    (Tfunction
                                                                    ((tptr tvoid) ::
                                                                    tulong ::
                                                                    (tptr tuchar) ::
                                                                    tulong ::
                                                                    (tptr tulong) ::
                                                                    nil) tint
                                                                    cc_default))))
                                                                    (Ssequence
                                                                    (Sset _t'51
                                                                    (Efield
                                                                    (Ederef
                                                                    (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                                                    (Tstruct __435 noattr))
                                                                    _ctx
                                                                    (tptr tvoid)))
                                                                    (Ssequence
                                                                    (Sset _t'52
                                                                    (Evar _variable_cursor tulong))
                                                                    (Ssequence
                                                                    (Sset _t'53
                                                                    (Evar _variable_cursor tulong))
                                                                    (Scall (Some _t'7)
                                                                    (Etempvar _t'50 (tptr 
                                                                    (Tfunction
                                                                    ((tptr tvoid) ::
                                                                    tulong ::
                                                                    (tptr tuchar) ::
                                                                    tulong ::
                                                                    (tptr tulong) ::
                                                                    nil) tint
                                                                    cc_default)))
                                                                    ((Etempvar _t'51 (tptr tvoid)) ::
                                                                    (Etempvar _i__1 tuint) ::
                                                                    (Ebinop Oadd
                                                                    (Etempvar _out (tptr tuchar))
                                                                    (Etempvar _t'52 tulong)
                                                                    (tptr tuchar)) ::
                                                                    (Ebinop Osub
                                                                    (Etempvar _out_cap tulong)
                                                                    (Etempvar _t'53 tulong)
                                                                    tulong) ::
                                                                    (Eaddrof
                                                                    (Evar _written tulong)
                                                                    (tptr tulong)) ::
                                                                    nil))))))
                                                                    (Sset _err
                                                                    (Etempvar _t'7 tint)))
                                                                    (Ssequence
                                                                    (Sifthenelse 
                                                                    (Ebinop Oeq
                                                                    (Etempvar _err tint)
                                                                    (Econst_int (Int.repr 0) tint)
                                                                    tint)
                                                                    (Ssequence
                                                                    (Ssequence
                                                                    (Sset _t'48
                                                                    (Evar _variable_cursor tulong))
                                                                    (Ssequence
                                                                    (Sset _t'49
                                                                    (Evar _written tulong))
                                                                    (Scall (Some _t'9)
                                                                    (Evar _ssz_internal_add_overflow_size 
                                                                    (Tfunction
                                                                    (tulong ::
                                                                    tulong ::
                                                                    (tptr tulong) ::
                                                                    nil)
                                                                    tbool
                                                                    cc_default))
                                                                    ((Etempvar _t'48 tulong) ::
                                                                    (Etempvar _t'49 tulong) ::
                                                                    (Eaddrof
                                                                    (Evar _variable_cursor tulong)
                                                                    (tptr tulong)) ::
                                                                    nil))))
                                                                    (Sset _t'8
                                                                    (Ecast
                                                                    (Etempvar _t'9 tbool)
                                                                    tbool)))
                                                                    (Sset _t'8
                                                                    (Econst_int (Int.repr 0) tint)))
                                                                    (Sifthenelse (Etempvar _t'8 tint)
                                                                    (Sset _err
                                                                    (Econst_int (Int.repr 3) tint))
                                                                    Sskip))))))))))))))
                                            (Ssequence
                                              (Sassign
                                                (Evar _next_fixed_cursor__1 tulong)
                                                (Econst_int (Int.repr 0) tuint))
                                              (Ssequence
                                                (Scall (Some _t'15)
                                                  (Evar _ssz_internal_add_overflow_size 
                                                  (Tfunction
                                                    (tulong :: tulong ::
                                                     (tptr tulong) :: nil)
                                                    tbool cc_default))
                                                  ((Etempvar _fixed_cursor tulong) ::
                                                   (Etempvar _fixed_size__1 tulong) ::
                                                   (Eaddrof
                                                     (Evar _next_fixed_cursor__1 tulong)
                                                     (tptr tulong)) :: nil))
                                                (Sifthenelse (Etempvar _t'15 tbool)
                                                  (Sset _err
                                                    (Econst_int (Int.repr 3) tint))
                                                  (Ssequence
                                                    (Ssequence
                                                      (Sset _t'43
                                                        (Evar _next_fixed_cursor__1 tulong))
                                                      (Ssequence
                                                        (Sset _t'44
                                                          (Evar _fixed_region tulong))
                                                        (Sifthenelse 
                                                          (Ebinop Ogt
                                                            (Etempvar _t'43 tulong)
                                                            (Etempvar _t'44 tulong)
                                                            tint)
                                                          (Sset _t'14
                                                            (Econst_int (Int.repr 1) tint))
                                                          (Ssequence
                                                            (Sset _t'45
                                                              (Evar _next_fixed_cursor__1 tulong))
                                                            (Sset _t'14
                                                              (Ecast
                                                                (Ebinop Ogt
                                                                  (Etempvar _t'45 tulong)
                                                                  (Etempvar _out_cap tulong)
                                                                  tint)
                                                                tbool))))))
                                                    (Sifthenelse (Etempvar _t'14 tint)
                                                      (Sset _err
                                                        (Econst_int (Int.repr 8) tint))
                                                      (Ssequence
                                                        (Ssequence
                                                          (Ssequence
                                                            (Sset _t'41
                                                              (Efield
                                                                (Ederef
                                                                  (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                                                  (Tstruct __435 noattr))
                                                                _write
                                                                (tptr 
                                                                (Tfunction
                                                                  ((tptr tvoid) ::
                                                                   tulong ::
                                                                   (tptr tuchar) ::
                                                                   tulong ::
                                                                   (tptr tulong) ::
                                                                   nil) tint
                                                                  cc_default))))
                                                            (Ssequence
                                                              (Sset _t'42
                                                                (Efield
                                                                  (Ederef
                                                                    (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                                                    (Tstruct __435 noattr))
                                                                  _ctx
                                                                  (tptr tvoid)))
                                                              (Scall (Some _t'12)
                                                                (Etempvar _t'41 (tptr 
                                                                (Tfunction
                                                                  ((tptr tvoid) ::
                                                                   tulong ::
                                                                   (tptr tuchar) ::
                                                                   tulong ::
                                                                   (tptr tulong) ::
                                                                   nil) tint
                                                                  cc_default)))
                                                                ((Etempvar _t'42 (tptr tvoid)) ::
                                                                 (Etempvar _i__1 tuint) ::
                                                                 (Ebinop Oadd
                                                                   (Etempvar _out (tptr tuchar))
                                                                   (Etempvar _fixed_cursor tulong)
                                                                   (tptr tuchar)) ::
                                                                 (Etempvar _fixed_size__1 tulong) ::
                                                                 (Eaddrof
                                                                   (Evar _written tulong)
                                                                   (tptr tulong)) ::
                                                                 nil))))
                                                          (Sset _err
                                                            (Etempvar _t'12 tint)))
                                                        (Ssequence
                                                          (Sifthenelse 
                                                            (Ebinop Oeq
                                                              (Etempvar _err tint)
                                                              (Econst_int (Int.repr 0) tint)
                                                              tint)
                                                            (Ssequence
                                                              (Sset _t'40
                                                                (Evar _written tulong))
                                                              (Sset _t'13
                                                                (Ecast
                                                                  (Ebinop One
                                                                    (Etempvar _t'40 tulong)
                                                                    (Etempvar _fixed_size__1 tulong)
                                                                    tint)
                                                                  tbool)))
                                                            (Sset _t'13
                                                              (Econst_int (Int.repr 0) tint)))
                                                          (Sifthenelse (Etempvar _t'13 tint)
                                                            (Sset _err
                                                              (Econst_int (Int.repr 8) tint))
                                                            (Sifthenelse 
                                                              (Ebinop Oeq
                                                                (Etempvar _err tint)
                                                                (Econst_int (Int.repr 0) tint)
                                                                tint)
                                                              (Sset _fixed_cursor
                                                                (Evar _next_fixed_cursor__1 tulong))
                                                              Sskip)))))))))))))
                                    (Sset _i__1
                                      (Ebinop Oadd (Etempvar _i__1 tuint)
                                        (Econst_int (Int.repr 1) tint) tuint))))
                                (Ssequence
                                  (Ssequence
                                    (Sifthenelse (Ebinop Oeq
                                                   (Etempvar _err tint)
                                                   (Econst_int (Int.repr 0) tint)
                                                   tint)
                                      (Ssequence
                                        (Sset _t'39
                                          (Evar _fixed_region tulong))
                                        (Sset _t'16
                                          (Ecast
                                            (Ebinop One
                                              (Etempvar _fixed_cursor tulong)
                                              (Etempvar _t'39 tulong) tint)
                                            tbool)))
                                      (Sset _t'16
                                        (Econst_int (Int.repr 0) tint)))
                                    (Sifthenelse (Etempvar _t'16 tint)
                                      (Sset _err
                                        (Econst_int (Int.repr 8) tint))
                                      Sskip))
                                  (Ssequence
                                    (Ssequence
                                      (Sifthenelse (Ebinop Oeq
                                                     (Etempvar _err tint)
                                                     (Econst_int (Int.repr 0) tint)
                                                     tint)
                                        (Ssequence
                                          (Sset _t'38
                                            (Evar _variable_cursor tulong))
                                          (Sset _t'17
                                            (Ecast
                                              (Ebinop Ogt
                                                (Etempvar _t'38 tulong)
                                                (Econst_int (Int.repr (-1)) tuint)
                                                tint) tbool)))
                                        (Sset _t'17
                                          (Econst_int (Int.repr 0) tint)))
                                      (Sifthenelse (Etempvar _t'17 tint)
                                        (Sset _err
                                          (Econst_int (Int.repr 3) tint))
                                        Sskip))
                                    (Sifthenelse (Ebinop Oeq
                                                   (Etempvar _err tint)
                                                   (Econst_int (Int.repr 0) tint)
                                                   tint)
                                      (Ssequence
                                        (Sset _t'37
                                          (Evar _variable_cursor tulong))
                                        (Sassign
                                          (Ederef
                                            (Etempvar _out_len (tptr tulong))
                                            tulong) (Etempvar _t'37 tulong)))
                                      Sskip)))))))))))
                Sskip))
            (Ssequence
              (Ssequence
                (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                               (Econst_int (Int.repr 0) tint) tint)
                  (Sset _t'24
                    (Ecast
                      (Ebinop Oeq (Etempvar _out (tptr tuchar))
                        (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid))
                        tint) tbool))
                  (Sset _t'24 (Econst_int (Int.repr 0) tint)))
                (Sifthenelse (Etempvar _t'24 tint)
                  (Ssequence
                    (Ssequence
                      (Sset _t'34 (Evar _fixed_region tulong))
                      (Sassign (Evar _total tulong) (Etempvar _t'34 tulong)))
                    (Ssequence
                      (Ssequence
                        (Sset _i__2 (Econst_int (Int.repr 0) tuint))
                        (Sloop
                          (Ssequence
                            (Ssequence
                              (Sifthenelse (Ebinop Olt (Etempvar _i__2 tuint)
                                             (Etempvar _field_count tuint)
                                             tint)
                                (Sset _t'19
                                  (Ecast
                                    (Ebinop Oeq (Etempvar _err tint)
                                      (Econst_int (Int.repr 0) tint) tint)
                                    tbool))
                                (Sset _t'19 (Econst_int (Int.repr 0) tint)))
                              (Sifthenelse (Etempvar _t'19 tint)
                                Sskip
                                Sbreak))
                            (Ssequence
                              (Sassign (Evar _encoded_len tulong)
                                (Econst_int (Int.repr 0) tuint))
                              (Ssequence
                                (Ssequence
                                  (Ssequence
                                    (Sset _t'32
                                      (Efield
                                        (Ederef
                                          (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                          (Tstruct __435 noattr)) _write
                                        (tptr (Tfunction
                                                ((tptr tvoid) :: tulong ::
                                                 (tptr tuchar) :: tulong ::
                                                 (tptr tulong) :: nil) tint
                                                cc_default))))
                                    (Ssequence
                                      (Sset _t'33
                                        (Efield
                                          (Ederef
                                            (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                            (Tstruct __435 noattr)) _ctx
                                          (tptr tvoid)))
                                      (Scall (Some _t'20)
                                        (Etempvar _t'32 (tptr (Tfunction
                                                                ((tptr tvoid) ::
                                                                 tulong ::
                                                                 (tptr tuchar) ::
                                                                 tulong ::
                                                                 (tptr tulong) ::
                                                                 nil) tint
                                                                cc_default)))
                                        ((Etempvar _t'33 (tptr tvoid)) ::
                                         (Etempvar _i__2 tuint) ::
                                         (Ecast
                                           (Econst_int (Int.repr 0) tint)
                                           (tptr tvoid)) ::
                                         (Econst_int (Int.repr 0) tuint) ::
                                         (Eaddrof (Evar _encoded_len tulong)
                                           (tptr tulong)) :: nil))))
                                  (Sset _err (Etempvar _t'20 tint)))
                                (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                                               (Econst_int (Int.repr 0) tint)
                                               tint)
                                  (Ssequence
                                    (Sset _t'27
                                      (Ederef
                                        (Ebinop Oadd
                                          (Etempvar _field_fixed_sizes (tptr tulong))
                                          (Etempvar _i__2 tuint)
                                          (tptr tulong)) tulong))
                                    (Sifthenelse (Ebinop Oeq
                                                   (Etempvar _t'27 tulong)
                                                   (Econst_int (Int.repr 0) tuint)
                                                   tint)
                                      (Ssequence
                                        (Ssequence
                                          (Sset _t'30 (Evar _total tulong))
                                          (Ssequence
                                            (Sset _t'31
                                              (Evar _encoded_len tulong))
                                            (Scall (Some _t'21)
                                              (Evar _ssz_internal_add_overflow_size 
                                              (Tfunction
                                                (tulong :: tulong ::
                                                 (tptr tulong) :: nil) tbool
                                                cc_default))
                                              ((Etempvar _t'30 tulong) ::
                                               (Etempvar _t'31 tulong) ::
                                               (Eaddrof (Evar _total tulong)
                                                 (tptr tulong)) :: nil))))
                                        (Sifthenelse (Etempvar _t'21 tbool)
                                          (Sset _err
                                            (Econst_int (Int.repr 3) tint))
                                          Sskip))
                                      (Ssequence
                                        (Sset _t'28
                                          (Evar _encoded_len tulong))
                                        (Ssequence
                                          (Sset _t'29
                                            (Ederef
                                              (Ebinop Oadd
                                                (Etempvar _field_fixed_sizes (tptr tulong))
                                                (Etempvar _i__2 tuint)
                                                (tptr tulong)) tulong))
                                          (Sifthenelse (Ebinop One
                                                         (Etempvar _t'28 tulong)
                                                         (Etempvar _t'29 tulong)
                                                         tint)
                                            (Sset _err
                                              (Econst_int (Int.repr 8) tint))
                                            Sskip)))))
                                  Sskip))))
                          (Sset _i__2
                            (Ebinop Oadd (Etempvar _i__2 tuint)
                              (Econst_int (Int.repr 1) tint) tuint))))
                      (Ssequence
                        (Ssequence
                          (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                                         (Econst_int (Int.repr 0) tint) tint)
                            (Ssequence
                              (Sset _t'26 (Evar _total tulong))
                              (Sset _t'22
                                (Ecast
                                  (Ebinop Ogt (Etempvar _t'26 tulong)
                                    (Econst_int (Int.repr (-1)) tuint) tint)
                                  tbool)))
                            (Sset _t'22 (Econst_int (Int.repr 0) tint)))
                          (Sifthenelse (Etempvar _t'22 tint)
                            (Sset _err (Econst_int (Int.repr 3) tint))
                            Sskip))
                        (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                                       (Econst_int (Int.repr 0) tint) tint)
                          (Ssequence
                            (Ssequence
                              (Sset _t'25 (Evar _total tulong))
                              (Scall (Some _t'23)
                                (Evar _ssz_internal_prepare_output (Tfunction
                                                                    (tulong ::
                                                                    (tptr tuchar) ::
                                                                    tulong ::
                                                                    (tptr tulong) ::
                                                                    nil) tint
                                                                    cc_default))
                                ((Etempvar _t'25 tulong) ::
                                 (Etempvar _out (tptr tuchar)) ::
                                 (Etempvar _out_cap tulong) ::
                                 (Etempvar _out_len (tptr tulong)) :: nil)))
                            (Sset _err (Etempvar _t'23 tint)))
                          Sskip))))
                  Sskip))
              (Sreturn (Some (Etempvar _err tint))))))))))
|}.

Definition f_ssz_serialize_union := {|
  fn_return := tint;
  fn_callconv := cc_default;
  fn_params := ((_selector, tuchar) :: (_option_count, tuint) ::
                (_has_none, tbool) ::
                (_codec, (tptr (Tstruct __435 noattr))) ::
                (_out, (tptr tuchar)) :: (_out_cap, tulong) ::
                (_out_len, (tptr tulong)) :: nil);
  fn_vars := ((_payload_len, tulong) :: (_total, tulong) ::
              (_written, tulong) :: nil);
  fn_temps := ((_err, tint) :: (_t'9, tint) :: (_t'8, tint) ::
               (_t'7, tint) :: (_t'6, tint) :: (_t'5, tint) ::
               (_t'4, tint) :: (_t'3, tint) :: (_t'2, tbool) ::
               (_t'1, tint) ::
               (_t'20,
                (tptr (Tfunction
                        ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong ::
                         (tptr tulong) :: nil) tint cc_default))) ::
               (_t'19, (tptr tvoid)) ::
               (_t'18,
                (tptr (Tfunction
                        ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong ::
                         (tptr tulong) :: nil) tint cc_default))) ::
               (_t'17, tulong) :: (_t'16, tulong) :: (_t'15, tulong) ::
               (_t'14, (tptr tvoid)) ::
               (_t'13,
                (tptr (Tfunction
                        ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong ::
                         (tptr tulong) :: nil) tint cc_default))) ::
               (_t'12, tulong) :: (_t'11, tulong) :: (_t'10, tulong) :: nil);
  fn_body :=
(Ssequence
  (Sassign (Evar _payload_len tulong) (Econst_int (Int.repr 0) tuint))
  (Ssequence
    (Sassign (Evar _total tulong) (Econst_int (Int.repr 1) tuint))
    (Ssequence
      (Sset _err (Econst_int (Int.repr 0) tint))
      (Ssequence
        (Sifthenelse (Ebinop Oeq (Etempvar _option_count tuint)
                       (Econst_int (Int.repr 0) tuint) tint)
          (Sset _err (Econst_int (Int.repr 5) tint))
          (Sifthenelse (Ebinop Ogt (Etempvar _option_count tuint)
                         (Econst_int (Int.repr 256) tuint) tint)
            (Sset _err (Econst_int (Int.repr 5) tint))
            (Ssequence
              (Sifthenelse (Etempvar _has_none tbool)
                (Sset _t'5
                  (Ecast
                    (Ebinop Olt (Etempvar _option_count tuint)
                      (Econst_int (Int.repr 2) tuint) tint) tbool))
                (Sset _t'5 (Econst_int (Int.repr 0) tint)))
              (Sifthenelse (Etempvar _t'5 tint)
                (Sset _err (Econst_int (Int.repr 5) tint))
                (Sifthenelse (Ebinop Oge
                               (Ecast (Etempvar _selector tuchar) tuint)
                               (Etempvar _option_count tuint) tint)
                  (Sset _err (Econst_int (Int.repr 9) tint))
                  (Ssequence
                    (Sifthenelse (Etempvar _has_none tbool)
                      (Sset _t'4
                        (Ecast
                          (Ebinop Oeq (Etempvar _selector tuchar)
                            (Econst_int (Int.repr 0) tuint) tint) tbool))
                      (Sset _t'4 (Econst_int (Int.repr 0) tint)))
                    (Sifthenelse (Eunop Onotbool (Etempvar _t'4 tint) tint)
                      (Ssequence
                        (Sifthenelse (Ebinop Oeq
                                       (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                       (Ecast (Econst_int (Int.repr 0) tint)
                                         (tptr tvoid)) tint)
                          (Sset _t'3 (Econst_int (Int.repr 1) tint))
                          (Ssequence
                            (Sset _t'20
                              (Efield
                                (Ederef
                                  (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                  (Tstruct __435 noattr)) _write
                                (tptr (Tfunction
                                        ((tptr tvoid) :: tulong ::
                                         (tptr tuchar) :: tulong ::
                                         (tptr tulong) :: nil) tint
                                        cc_default))))
                            (Sset _t'3
                              (Ecast
                                (Ebinop Oeq
                                  (Etempvar _t'20 (tptr (Tfunction
                                                          ((tptr tvoid) ::
                                                           tulong ::
                                                           (tptr tuchar) ::
                                                           tulong ::
                                                           (tptr tulong) ::
                                                           nil) tint
                                                          cc_default)))
                                  (Ecast (Econst_int (Int.repr 0) tint)
                                    (tptr tvoid)) tint) tbool))))
                        (Sifthenelse (Etempvar _t'3 tint)
                          (Sset _err (Econst_int (Int.repr 1) tint))
                          (Ssequence
                            (Ssequence
                              (Ssequence
                                (Sset _t'18
                                  (Efield
                                    (Ederef
                                      (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                      (Tstruct __435 noattr)) _write
                                    (tptr (Tfunction
                                            ((tptr tvoid) :: tulong ::
                                             (tptr tuchar) :: tulong ::
                                             (tptr tulong) :: nil) tint
                                            cc_default))))
                                (Ssequence
                                  (Sset _t'19
                                    (Efield
                                      (Ederef
                                        (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                        (Tstruct __435 noattr)) _ctx
                                      (tptr tvoid)))
                                  (Scall (Some _t'1)
                                    (Etempvar _t'18 (tptr (Tfunction
                                                            ((tptr tvoid) ::
                                                             tulong ::
                                                             (tptr tuchar) ::
                                                             tulong ::
                                                             (tptr tulong) ::
                                                             nil) tint
                                                            cc_default)))
                                    ((Etempvar _t'19 (tptr tvoid)) ::
                                     (Etempvar _selector tuchar) ::
                                     (Ecast (Econst_int (Int.repr 0) tint)
                                       (tptr tvoid)) ::
                                     (Econst_int (Int.repr 0) tuint) ::
                                     (Eaddrof (Evar _payload_len tulong)
                                       (tptr tulong)) :: nil))))
                              (Sset _err (Etempvar _t'1 tint)))
                            (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                                           (Econst_int (Int.repr 0) tint)
                                           tint)
                              (Ssequence
                                (Ssequence
                                  (Sset _t'16 (Evar _total tulong))
                                  (Ssequence
                                    (Sset _t'17 (Evar _payload_len tulong))
                                    (Scall (Some _t'2)
                                      (Evar _ssz_internal_add_overflow_size 
                                      (Tfunction
                                        (tulong :: tulong :: (tptr tulong) ::
                                         nil) tbool cc_default))
                                      ((Etempvar _t'16 tulong) ::
                                       (Etempvar _t'17 tulong) ::
                                       (Eaddrof (Evar _total tulong)
                                         (tptr tulong)) :: nil))))
                                (Sifthenelse (Etempvar _t'2 tbool)
                                  (Sset _err (Econst_int (Int.repr 3) tint))
                                  Sskip))
                              Sskip))))
                      Sskip)))))))
        (Ssequence
          (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                         (Econst_int (Int.repr 0) tint) tint)
            (Ssequence
              (Ssequence
                (Sset _t'15 (Evar _total tulong))
                (Scall (Some _t'6)
                  (Evar _ssz_internal_prepare_output (Tfunction
                                                       (tulong ::
                                                        (tptr tuchar) ::
                                                        tulong ::
                                                        (tptr tulong) :: nil)
                                                       tint cc_default))
                  ((Etempvar _t'15 tulong) ::
                   (Etempvar _out (tptr tuchar)) ::
                   (Etempvar _out_cap tulong) ::
                   (Etempvar _out_len (tptr tulong)) :: nil)))
              (Sset _err (Etempvar _t'6 tint)))
            Sskip)
          (Ssequence
            (Ssequence
              (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                             (Econst_int (Int.repr 0) tint) tint)
                (Sset _t'9
                  (Ecast
                    (Ebinop One (Etempvar _out (tptr tuchar))
                      (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid))
                      tint) tbool))
                (Sset _t'9 (Econst_int (Int.repr 0) tint)))
              (Sifthenelse (Etempvar _t'9 tint)
                (Ssequence
                  (Sassign
                    (Ederef
                      (Ebinop Oadd (Etempvar _out (tptr tuchar))
                        (Econst_int (Int.repr 0) tint) (tptr tuchar)) tuchar)
                    (Etempvar _selector tuchar))
                  (Ssequence
                    (Sset _t'10 (Evar _total tulong))
                    (Sifthenelse (Ebinop One (Etempvar _t'10 tulong)
                                   (Econst_int (Int.repr 1) tuint) tint)
                      (Ssequence
                        (Sassign (Evar _written tulong)
                          (Econst_int (Int.repr 0) tuint))
                        (Ssequence
                          (Ssequence
                            (Ssequence
                              (Sset _t'13
                                (Efield
                                  (Ederef
                                    (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                    (Tstruct __435 noattr)) _write
                                  (tptr (Tfunction
                                          ((tptr tvoid) :: tulong ::
                                           (tptr tuchar) :: tulong ::
                                           (tptr tulong) :: nil) tint
                                          cc_default))))
                              (Ssequence
                                (Sset _t'14
                                  (Efield
                                    (Ederef
                                      (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                      (Tstruct __435 noattr)) _ctx
                                    (tptr tvoid)))
                                (Scall (Some _t'7)
                                  (Etempvar _t'13 (tptr (Tfunction
                                                          ((tptr tvoid) ::
                                                           tulong ::
                                                           (tptr tuchar) ::
                                                           tulong ::
                                                           (tptr tulong) ::
                                                           nil) tint
                                                          cc_default)))
                                  ((Etempvar _t'14 (tptr tvoid)) ::
                                   (Etempvar _selector tuchar) ::
                                   (Ebinop Oadd (Etempvar _out (tptr tuchar))
                                     (Econst_int (Int.repr 1) tuint)
                                     (tptr tuchar)) ::
                                   (Ebinop Osub (Etempvar _out_cap tulong)
                                     (Econst_int (Int.repr 1) tuint) tulong) ::
                                   (Eaddrof (Evar _written tulong)
                                     (tptr tulong)) :: nil))))
                            (Sset _err (Etempvar _t'7 tint)))
                          (Ssequence
                            (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                                           (Econst_int (Int.repr 0) tint)
                                           tint)
                              (Ssequence
                                (Sset _t'11 (Evar _written tulong))
                                (Ssequence
                                  (Sset _t'12 (Evar _payload_len tulong))
                                  (Sset _t'8
                                    (Ecast
                                      (Ebinop One (Etempvar _t'11 tulong)
                                        (Etempvar _t'12 tulong) tint) tbool))))
                              (Sset _t'8 (Econst_int (Int.repr 0) tint)))
                            (Sifthenelse (Etempvar _t'8 tint)
                              (Sset _err (Econst_int (Int.repr 8) tint))
                              Sskip))))
                      Sskip)))
                Sskip))
            (Sreturn (Some (Etempvar _err tint)))))))))
|}.

Definition f_ssz_serialize_compatible_union := {|
  fn_return := tint;
  fn_callconv := cc_default;
  fn_params := ((_selector, tuchar) :: (_allowed_selectors, (tptr tuchar)) ::
                (_allowed_selector_count, tuint) ::
                (_codec, (tptr (Tstruct __435 noattr))) ::
                (_out, (tptr tuchar)) :: (_out_cap, tulong) ::
                (_out_len, (tptr tulong)) :: nil);
  fn_vars := ((_payload_len, tulong) :: (_total, tulong) ::
              (_written, tulong) :: nil);
  fn_temps := ((_err, tint) :: (_t'11, tbool) :: (_t'10, tint) ::
               (_t'9, tint) :: (_t'8, tint) :: (_t'7, tint) ::
               (_t'6, tint) :: (_t'5, tint) :: (_t'4, tint) ::
               (_t'3, tbool) :: (_t'2, tint) :: (_t'1, tint) ::
               (_t'21,
                (tptr (Tfunction
                        ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong ::
                         (tptr tulong) :: nil) tint cc_default))) ::
               (_t'20, (tptr tvoid)) ::
               (_t'19,
                (tptr (Tfunction
                        ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong ::
                         (tptr tulong) :: nil) tint cc_default))) ::
               (_t'18, tulong) :: (_t'17, tulong) :: (_t'16, tulong) ::
               (_t'15, (tptr tvoid)) ::
               (_t'14,
                (tptr (Tfunction
                        ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong ::
                         (tptr tulong) :: nil) tint cc_default))) ::
               (_t'13, tulong) :: (_t'12, tulong) :: nil);
  fn_body :=
(Ssequence
  (Sassign (Evar _payload_len tulong) (Econst_int (Int.repr 0) tuint))
  (Ssequence
    (Sassign (Evar _total tulong) (Econst_int (Int.repr 1) tuint))
    (Ssequence
      (Ssequence
        (Scall (Some _t'1)
          (Evar _ssz_internal_validate_compatible_union_schema (Tfunction
                                                                 ((tptr tuchar) ::
                                                                  tuint ::
                                                                  nil) tint
                                                                 cc_default))
          ((Etempvar _allowed_selectors (tptr tuchar)) ::
           (Etempvar _allowed_selector_count tuint) :: nil))
        (Sset _err (Etempvar _t'1 tint)))
      (Ssequence
        (Sifthenelse (Ebinop One (Etempvar _err tint)
                       (Econst_int (Int.repr 0) tint) tint)
          Sskip
          (Ssequence
            (Ssequence
              (Sifthenelse (Ebinop Oeq (Etempvar _selector tuchar)
                             (Econst_int (Int.repr 0) tuint) tint)
                (Sset _t'9 (Econst_int (Int.repr 1) tint))
                (Sset _t'9
                  (Ecast
                    (Ebinop Ogt (Etempvar _selector tuchar)
                      (Econst_int (Int.repr 127) tuint) tint) tbool)))
              (Sifthenelse (Etempvar _t'9 tint)
                (Sset _t'10 (Econst_int (Int.repr 1) tint))
                (Ssequence
                  (Scall (Some _t'11)
                    (Evar _ssz_internal_selector_allowed (Tfunction
                                                           (tuchar ::
                                                            (tptr tuchar) ::
                                                            tuint :: nil)
                                                           tbool cc_default))
                    ((Etempvar _selector tuchar) ::
                     (Etempvar _allowed_selectors (tptr tuchar)) ::
                     (Etempvar _allowed_selector_count tuint) :: nil))
                  (Sset _t'10
                    (Ecast (Eunop Onotbool (Etempvar _t'11 tbool) tint)
                      tbool)))))
            (Sifthenelse (Etempvar _t'10 tint)
              (Sset _err (Econst_int (Int.repr 9) tint))
              (Ssequence
                (Sifthenelse (Ebinop Oeq
                               (Etempvar _codec (tptr (Tstruct __435 noattr)))
                               (Ecast (Econst_int (Int.repr 0) tint)
                                 (tptr tvoid)) tint)
                  (Sset _t'8 (Econst_int (Int.repr 1) tint))
                  (Ssequence
                    (Sset _t'21
                      (Efield
                        (Ederef
                          (Etempvar _codec (tptr (Tstruct __435 noattr)))
                          (Tstruct __435 noattr)) _write
                        (tptr (Tfunction
                                ((tptr tvoid) :: tulong :: (tptr tuchar) ::
                                 tulong :: (tptr tulong) :: nil) tint
                                cc_default))))
                    (Sset _t'8
                      (Ecast
                        (Ebinop Oeq
                          (Etempvar _t'21 (tptr (Tfunction
                                                  ((tptr tvoid) :: tulong ::
                                                   (tptr tuchar) :: tulong ::
                                                   (tptr tulong) :: nil) tint
                                                  cc_default)))
                          (Ecast (Econst_int (Int.repr 0) tint) (tptr tvoid))
                          tint) tbool))))
                (Sifthenelse (Etempvar _t'8 tint)
                  (Sset _err (Econst_int (Int.repr 1) tint))
                  (Ssequence
                    (Ssequence
                      (Ssequence
                        (Sset _t'19
                          (Efield
                            (Ederef
                              (Etempvar _codec (tptr (Tstruct __435 noattr)))
                              (Tstruct __435 noattr)) _write
                            (tptr (Tfunction
                                    ((tptr tvoid) :: tulong ::
                                     (tptr tuchar) :: tulong ::
                                     (tptr tulong) :: nil) tint cc_default))))
                        (Ssequence
                          (Sset _t'20
                            (Efield
                              (Ederef
                                (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                (Tstruct __435 noattr)) _ctx (tptr tvoid)))
                          (Scall (Some _t'2)
                            (Etempvar _t'19 (tptr (Tfunction
                                                    ((tptr tvoid) ::
                                                     tulong ::
                                                     (tptr tuchar) ::
                                                     tulong ::
                                                     (tptr tulong) :: nil)
                                                    tint cc_default)))
                            ((Etempvar _t'20 (tptr tvoid)) ::
                             (Etempvar _selector tuchar) ::
                             (Ecast (Econst_int (Int.repr 0) tint)
                               (tptr tvoid)) ::
                             (Econst_int (Int.repr 0) tuint) ::
                             (Eaddrof (Evar _payload_len tulong)
                               (tptr tulong)) :: nil))))
                      (Sset _err (Etempvar _t'2 tint)))
                    (Ssequence
                      (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                                     (Econst_int (Int.repr 0) tint) tint)
                        (Ssequence
                          (Ssequence
                            (Sset _t'17 (Evar _total tulong))
                            (Ssequence
                              (Sset _t'18 (Evar _payload_len tulong))
                              (Scall (Some _t'3)
                                (Evar _ssz_internal_add_overflow_size 
                                (Tfunction
                                  (tulong :: tulong :: (tptr tulong) :: nil)
                                  tbool cc_default))
                                ((Etempvar _t'17 tulong) ::
                                 (Etempvar _t'18 tulong) ::
                                 (Eaddrof (Evar _total tulong) (tptr tulong)) ::
                                 nil))))
                          (Sifthenelse (Etempvar _t'3 tbool)
                            (Sset _err (Econst_int (Int.repr 3) tint))
                            Sskip))
                        Sskip)
                      (Ssequence
                        (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                                       (Econst_int (Int.repr 0) tint) tint)
                          (Ssequence
                            (Ssequence
                              (Sset _t'16 (Evar _total tulong))
                              (Scall (Some _t'4)
                                (Evar _ssz_internal_prepare_output (Tfunction
                                                                    (tulong ::
                                                                    (tptr tuchar) ::
                                                                    tulong ::
                                                                    (tptr tulong) ::
                                                                    nil) tint
                                                                    cc_default))
                                ((Etempvar _t'16 tulong) ::
                                 (Etempvar _out (tptr tuchar)) ::
                                 (Etempvar _out_cap tulong) ::
                                 (Etempvar _out_len (tptr tulong)) :: nil)))
                            (Sset _err (Etempvar _t'4 tint)))
                          Sskip)
                        (Ssequence
                          (Sifthenelse (Ebinop Oeq (Etempvar _err tint)
                                         (Econst_int (Int.repr 0) tint) tint)
                            (Sset _t'7
                              (Ecast
                                (Ebinop One (Etempvar _out (tptr tuchar))
                                  (Ecast (Econst_int (Int.repr 0) tint)
                                    (tptr tvoid)) tint) tbool))
                            (Sset _t'7 (Econst_int (Int.repr 0) tint)))
                          (Sifthenelse (Etempvar _t'7 tint)
                            (Ssequence
                              (Sassign (Evar _written tulong)
                                (Econst_int (Int.repr 0) tuint))
                              (Ssequence
                                (Sassign
                                  (Ederef
                                    (Ebinop Oadd
                                      (Etempvar _out (tptr tuchar))
                                      (Econst_int (Int.repr 0) tint)
                                      (tptr tuchar)) tuchar)
                                  (Etempvar _selector tuchar))
                                (Ssequence
                                  (Ssequence
                                    (Ssequence
                                      (Sset _t'14
                                        (Efield
                                          (Ederef
                                            (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                            (Tstruct __435 noattr)) _write
                                          (tptr (Tfunction
                                                  ((tptr tvoid) :: tulong ::
                                                   (tptr tuchar) :: tulong ::
                                                   (tptr tulong) :: nil) tint
                                                  cc_default))))
                                      (Ssequence
                                        (Sset _t'15
                                          (Efield
                                            (Ederef
                                              (Etempvar _codec (tptr (Tstruct __435 noattr)))
                                              (Tstruct __435 noattr)) _ctx
                                            (tptr tvoid)))
                                        (Scall (Some _t'5)
                                          (Etempvar _t'14 (tptr (Tfunction
                                                                  ((tptr tvoid) ::
                                                                   tulong ::
                                                                   (tptr tuchar) ::
                                                                   tulong ::
                                                                   (tptr tulong) ::
                                                                   nil) tint
                                                                  cc_default)))
                                          ((Etempvar _t'15 (tptr tvoid)) ::
                                           (Etempvar _selector tuchar) ::
                                           (Ebinop Oadd
                                             (Etempvar _out (tptr tuchar))
                                             (Econst_int (Int.repr 1) tuint)
                                             (tptr tuchar)) ::
                                           (Ebinop Osub
                                             (Etempvar _out_cap tulong)
                                             (Econst_int (Int.repr 1) tuint)
                                             tulong) ::
                                           (Eaddrof (Evar _written tulong)
                                             (tptr tulong)) :: nil))))
                                    (Sset _err (Etempvar _t'5 tint)))
                                  (Ssequence
                                    (Sifthenelse (Ebinop Oeq
                                                   (Etempvar _err tint)
                                                   (Econst_int (Int.repr 0) tint)
                                                   tint)
                                      (Ssequence
                                        (Sset _t'12 (Evar _written tulong))
                                        (Ssequence
                                          (Sset _t'13
                                            (Evar _payload_len tulong))
                                          (Sset _t'6
                                            (Ecast
                                              (Ebinop One
                                                (Etempvar _t'12 tulong)
                                                (Etempvar _t'13 tulong) tint)
                                              tbool))))
                                      (Sset _t'6
                                        (Econst_int (Int.repr 0) tint)))
                                    (Sifthenelse (Etempvar _t'6 tint)
                                      (Sset _err
                                        (Econst_int (Int.repr 8) tint))
                                      Sskip)))))
                            Sskip))))))))))
        (Sreturn (Some (Etempvar _err tint)))))))
|}.

Definition composites : list composite_definition :=
(Composite __395 Struct
   (Member_plain _bytes (tarray tuchar 32) :: nil)
   noattr ::
 Composite __435 Struct
   (Member_plain _ctx (tptr tvoid) ::
    Member_plain _write
      (tptr (Tfunction
              ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong ::
               (tptr tulong) :: nil) tint cc_default)) ::
    Member_plain _read
      (tptr (Tfunction
              ((tptr tvoid) :: tulong :: (tptr tuchar) :: tulong :: nil) tint
              cc_default)) ::
    Member_plain _root
      (tptr (Tfunction
              ((tptr tvoid) :: tulong :: (tptr (Tstruct __395 noattr)) ::
               nil) tint cc_default)) :: nil)
   noattr ::
 Composite __449 Struct
   (Member_plain _field_fixed_sizes (tptr tulong) ::
    Member_plain _field_count tuint :: nil)
   noattr :: nil).

Definition global_definitions : list (ident * globdef fundef type) :=
((___compcert_va_int32,
   Gfun(External (EF_runtime "__compcert_va_int32"
                   (mksignature (AST.Xptr :: nil) AST.Xint cc_default))
     ((tptr tvoid) :: nil) tuint cc_default)) ::
 (___compcert_va_int64,
   Gfun(External (EF_runtime "__compcert_va_int64"
                   (mksignature (AST.Xptr :: nil) AST.Xlong cc_default))
     ((tptr tvoid) :: nil) tulong cc_default)) ::
 (___compcert_va_float64,
   Gfun(External (EF_runtime "__compcert_va_float64"
                   (mksignature (AST.Xptr :: nil) AST.Xfloat cc_default))
     ((tptr tvoid) :: nil) tdouble cc_default)) ::
 (___compcert_va_composite,
   Gfun(External (EF_runtime "__compcert_va_composite"
                   (mksignature (AST.Xptr :: AST.Xlong :: nil) AST.Xptr
                     cc_default)) ((tptr tvoid) :: tulong :: nil)
     (tptr tvoid) cc_default)) ::
 (___compcert_i64_dtos,
   Gfun(External (EF_runtime "__compcert_i64_dtos"
                   (mksignature (AST.Xfloat :: nil) AST.Xlong cc_default))
     (tdouble :: nil) tlong cc_default)) ::
 (___compcert_i64_dtou,
   Gfun(External (EF_runtime "__compcert_i64_dtou"
                   (mksignature (AST.Xfloat :: nil) AST.Xlong cc_default))
     (tdouble :: nil) tulong cc_default)) ::
 (___compcert_i64_stod,
   Gfun(External (EF_runtime "__compcert_i64_stod"
                   (mksignature (AST.Xlong :: nil) AST.Xfloat cc_default))
     (tlong :: nil) tdouble cc_default)) ::
 (___compcert_i64_utod,
   Gfun(External (EF_runtime "__compcert_i64_utod"
                   (mksignature (AST.Xlong :: nil) AST.Xfloat cc_default))
     (tulong :: nil) tdouble cc_default)) ::
 (___compcert_i64_stof,
   Gfun(External (EF_runtime "__compcert_i64_stof"
                   (mksignature (AST.Xlong :: nil) AST.Xsingle cc_default))
     (tlong :: nil) tfloat cc_default)) ::
 (___compcert_i64_utof,
   Gfun(External (EF_runtime "__compcert_i64_utof"
                   (mksignature (AST.Xlong :: nil) AST.Xsingle cc_default))
     (tulong :: nil) tfloat cc_default)) ::
 (___compcert_i64_sdiv,
   Gfun(External (EF_runtime "__compcert_i64_sdiv"
                   (mksignature (AST.Xlong :: AST.Xlong :: nil) AST.Xlong
                     cc_default)) (tlong :: tlong :: nil) tlong cc_default)) ::
 (___compcert_i64_udiv,
   Gfun(External (EF_runtime "__compcert_i64_udiv"
                   (mksignature (AST.Xlong :: AST.Xlong :: nil) AST.Xlong
                     cc_default)) (tulong :: tulong :: nil) tulong
     cc_default)) ::
 (___compcert_i64_smod,
   Gfun(External (EF_runtime "__compcert_i64_smod"
                   (mksignature (AST.Xlong :: AST.Xlong :: nil) AST.Xlong
                     cc_default)) (tlong :: tlong :: nil) tlong cc_default)) ::
 (___compcert_i64_umod,
   Gfun(External (EF_runtime "__compcert_i64_umod"
                   (mksignature (AST.Xlong :: AST.Xlong :: nil) AST.Xlong
                     cc_default)) (tulong :: tulong :: nil) tulong
     cc_default)) ::
 (___compcert_i64_shl,
   Gfun(External (EF_runtime "__compcert_i64_shl"
                   (mksignature (AST.Xlong :: AST.Xint :: nil) AST.Xlong
                     cc_default)) (tlong :: tint :: nil) tlong cc_default)) ::
 (___compcert_i64_shr,
   Gfun(External (EF_runtime "__compcert_i64_shr"
                   (mksignature (AST.Xlong :: AST.Xint :: nil) AST.Xlong
                     cc_default)) (tulong :: tint :: nil) tulong cc_default)) ::
 (___compcert_i64_sar,
   Gfun(External (EF_runtime "__compcert_i64_sar"
                   (mksignature (AST.Xlong :: AST.Xint :: nil) AST.Xlong
                     cc_default)) (tlong :: tint :: nil) tlong cc_default)) ::
 (___compcert_i64_smulh,
   Gfun(External (EF_runtime "__compcert_i64_smulh"
                   (mksignature (AST.Xlong :: AST.Xlong :: nil) AST.Xlong
                     cc_default)) (tlong :: tlong :: nil) tlong cc_default)) ::
 (___compcert_i64_umulh,
   Gfun(External (EF_runtime "__compcert_i64_umulh"
                   (mksignature (AST.Xlong :: AST.Xlong :: nil) AST.Xlong
                     cc_default)) (tulong :: tulong :: nil) tulong
     cc_default)) ::
 (___builtin_bswap64,
   Gfun(External (EF_builtin "__builtin_bswap64"
                   (mksignature (AST.Xlong :: nil) AST.Xlong cc_default))
     (tulong :: nil) tulong cc_default)) ::
 (___builtin_bswap,
   Gfun(External (EF_builtin "__builtin_bswap"
                   (mksignature (AST.Xint :: nil) AST.Xint cc_default))
     (tuint :: nil) tuint cc_default)) ::
 (___builtin_bswap32,
   Gfun(External (EF_builtin "__builtin_bswap32"
                   (mksignature (AST.Xint :: nil) AST.Xint cc_default))
     (tuint :: nil) tuint cc_default)) ::
 (___builtin_bswap16,
   Gfun(External (EF_builtin "__builtin_bswap16"
                   (mksignature (AST.Xint16unsigned :: nil)
                     AST.Xint16unsigned cc_default)) (tushort :: nil) tushort
     cc_default)) ::
 (___builtin_clz,
   Gfun(External (EF_builtin "__builtin_clz"
                   (mksignature (AST.Xint :: nil) AST.Xint cc_default))
     (tuint :: nil) tint cc_default)) ::
 (___builtin_clzl,
   Gfun(External (EF_builtin "__builtin_clzl"
                   (mksignature (AST.Xlong :: nil) AST.Xint cc_default))
     (tulong :: nil) tint cc_default)) ::
 (___builtin_clzll,
   Gfun(External (EF_builtin "__builtin_clzll"
                   (mksignature (AST.Xlong :: nil) AST.Xint cc_default))
     (tulong :: nil) tint cc_default)) ::
 (___builtin_ctz,
   Gfun(External (EF_builtin "__builtin_ctz"
                   (mksignature (AST.Xint :: nil) AST.Xint cc_default))
     (tuint :: nil) tint cc_default)) ::
 (___builtin_ctzl,
   Gfun(External (EF_builtin "__builtin_ctzl"
                   (mksignature (AST.Xlong :: nil) AST.Xint cc_default))
     (tulong :: nil) tint cc_default)) ::
 (___builtin_ctzll,
   Gfun(External (EF_builtin "__builtin_ctzll"
                   (mksignature (AST.Xlong :: nil) AST.Xint cc_default))
     (tulong :: nil) tint cc_default)) ::
 (___builtin_fabs,
   Gfun(External (EF_builtin "__builtin_fabs"
                   (mksignature (AST.Xfloat :: nil) AST.Xfloat cc_default))
     (tdouble :: nil) tdouble cc_default)) ::
 (___builtin_fabsf,
   Gfun(External (EF_builtin "__builtin_fabsf"
                   (mksignature (AST.Xsingle :: nil) AST.Xsingle cc_default))
     (tfloat :: nil) tfloat cc_default)) ::
 (___builtin_fsqrt,
   Gfun(External (EF_builtin "__builtin_fsqrt"
                   (mksignature (AST.Xfloat :: nil) AST.Xfloat cc_default))
     (tdouble :: nil) tdouble cc_default)) ::
 (___builtin_sqrt,
   Gfun(External (EF_builtin "__builtin_sqrt"
                   (mksignature (AST.Xfloat :: nil) AST.Xfloat cc_default))
     (tdouble :: nil) tdouble cc_default)) ::
 (___builtin_memcpy_aligned,
   Gfun(External (EF_builtin "__builtin_memcpy_aligned"
                   (mksignature
                     (AST.Xptr :: AST.Xptr :: AST.Xlong :: AST.Xlong :: nil)
                     AST.Xvoid cc_default))
     ((tptr tvoid) :: (tptr tvoid) :: tulong :: tulong :: nil) tvoid
     cc_default)) ::
 (___builtin_sel,
   Gfun(External (EF_builtin "__builtin_sel"
                   (mksignature (AST.Xbool :: nil) AST.Xvoid
                     {|cc_vararg:=(Some 1); cc_unproto:=false; cc_structret:=false|}))
     (tbool :: nil) tvoid
     {|cc_vararg:=(Some 1); cc_unproto:=false; cc_structret:=false|})) ::
 (___builtin_annot,
   Gfun(External (EF_builtin "__builtin_annot"
                   (mksignature (AST.Xptr :: nil) AST.Xvoid
                     {|cc_vararg:=(Some 1); cc_unproto:=false; cc_structret:=false|}))
     ((tptr tschar) :: nil) tvoid
     {|cc_vararg:=(Some 1); cc_unproto:=false; cc_structret:=false|})) ::
 (___builtin_annot_intval,
   Gfun(External (EF_builtin "__builtin_annot_intval"
                   (mksignature (AST.Xptr :: AST.Xint :: nil) AST.Xint
                     cc_default)) ((tptr tschar) :: tint :: nil) tint
     cc_default)) ::
 (___builtin_membar,
   Gfun(External (EF_builtin "__builtin_membar"
                   (mksignature nil AST.Xvoid cc_default)) nil tvoid
     cc_default)) ::
 (___builtin_va_start,
   Gfun(External (EF_builtin "__builtin_va_start"
                   (mksignature (AST.Xptr :: nil) AST.Xvoid cc_default))
     ((tptr tvoid) :: nil) tvoid cc_default)) ::
 (___builtin_va_arg,
   Gfun(External (EF_builtin "__builtin_va_arg"
                   (mksignature (AST.Xptr :: AST.Xint :: nil) AST.Xvoid
                     cc_default)) ((tptr tvoid) :: tuint :: nil) tvoid
     cc_default)) ::
 (___builtin_va_copy,
   Gfun(External (EF_builtin "__builtin_va_copy"
                   (mksignature (AST.Xptr :: AST.Xptr :: nil) AST.Xvoid
                     cc_default)) ((tptr tvoid) :: (tptr tvoid) :: nil) tvoid
     cc_default)) ::
 (___builtin_va_end,
   Gfun(External (EF_builtin "__builtin_va_end"
                   (mksignature (AST.Xptr :: nil) AST.Xvoid cc_default))
     ((tptr tvoid) :: nil) tvoid cc_default)) ::
 (___builtin_unreachable,
   Gfun(External (EF_builtin "__builtin_unreachable"
                   (mksignature nil AST.Xvoid cc_default)) nil tvoid
     cc_default)) ::
 (___builtin_expect,
   Gfun(External (EF_builtin "__builtin_expect"
                   (mksignature (AST.Xlong :: AST.Xlong :: nil) AST.Xlong
                     cc_default)) (tlong :: tlong :: nil) tlong cc_default)) ::
 (___builtin_cls,
   Gfun(External (EF_builtin "__builtin_cls"
                   (mksignature (AST.Xint :: nil) AST.Xint cc_default))
     (tint :: nil) tint cc_default)) ::
 (___builtin_clsl,
   Gfun(External (EF_builtin "__builtin_clsl"
                   (mksignature (AST.Xlong :: nil) AST.Xint cc_default))
     (tlong :: nil) tint cc_default)) ::
 (___builtin_clsll,
   Gfun(External (EF_builtin "__builtin_clsll"
                   (mksignature (AST.Xlong :: nil) AST.Xint cc_default))
     (tlong :: nil) tint cc_default)) ::
 (___builtin_fmadd,
   Gfun(External (EF_builtin "__builtin_fmadd"
                   (mksignature
                     (AST.Xfloat :: AST.Xfloat :: AST.Xfloat :: nil)
                     AST.Xfloat cc_default))
     (tdouble :: tdouble :: tdouble :: nil) tdouble cc_default)) ::
 (___builtin_fmsub,
   Gfun(External (EF_builtin "__builtin_fmsub"
                   (mksignature
                     (AST.Xfloat :: AST.Xfloat :: AST.Xfloat :: nil)
                     AST.Xfloat cc_default))
     (tdouble :: tdouble :: tdouble :: nil) tdouble cc_default)) ::
 (___builtin_fnmadd,
   Gfun(External (EF_builtin "__builtin_fnmadd"
                   (mksignature
                     (AST.Xfloat :: AST.Xfloat :: AST.Xfloat :: nil)
                     AST.Xfloat cc_default))
     (tdouble :: tdouble :: tdouble :: nil) tdouble cc_default)) ::
 (___builtin_fnmsub,
   Gfun(External (EF_builtin "__builtin_fnmsub"
                   (mksignature
                     (AST.Xfloat :: AST.Xfloat :: AST.Xfloat :: nil)
                     AST.Xfloat cc_default))
     (tdouble :: tdouble :: tdouble :: nil) tdouble cc_default)) ::
 (___builtin_fmax,
   Gfun(External (EF_builtin "__builtin_fmax"
                   (mksignature (AST.Xfloat :: AST.Xfloat :: nil) AST.Xfloat
                     cc_default)) (tdouble :: tdouble :: nil) tdouble
     cc_default)) ::
 (___builtin_fmin,
   Gfun(External (EF_builtin "__builtin_fmin"
                   (mksignature (AST.Xfloat :: AST.Xfloat :: nil) AST.Xfloat
                     cc_default)) (tdouble :: tdouble :: nil) tdouble
     cc_default)) ::
 (___builtin_debug,
   Gfun(External (EF_external "__builtin_debug"
                   (mksignature (AST.Xint :: nil) AST.Xvoid
                     {|cc_vararg:=(Some 1); cc_unproto:=false; cc_structret:=false|}))
     (tint :: nil) tvoid
     {|cc_vararg:=(Some 1); cc_unproto:=false; cc_structret:=false|})) ::
 (_memcpy,
   Gfun(External (EF_external "memcpy"
                   (mksignature (AST.Xptr :: AST.Xptr :: AST.Xlong :: nil)
                     AST.Xptr cc_default))
     ((tptr tvoid) :: (tptr tvoid) :: tulong :: nil) (tptr tvoid)
     cc_default)) ::
 (_memset,
   Gfun(External (EF_external "memset"
                   (mksignature (AST.Xptr :: AST.Xint :: AST.Xlong :: nil)
                     AST.Xptr cc_default))
     ((tptr tvoid) :: tint :: tulong :: nil) (tptr tvoid) cc_default)) ::
 (_ssz_internal_write_u16_le,
   Gfun(External (EF_external "ssz_internal_write_u16_le"
                   (mksignature (AST.Xptr :: AST.Xint16unsigned :: nil)
                     AST.Xvoid cc_default)) ((tptr tuchar) :: tushort :: nil)
     tvoid cc_default)) ::
 (_ssz_internal_write_u32_le,
   Gfun(External (EF_external "ssz_internal_write_u32_le"
                   (mksignature (AST.Xptr :: AST.Xint :: nil) AST.Xvoid
                     cc_default)) ((tptr tuchar) :: tuint :: nil) tvoid
     cc_default)) ::
 (_ssz_internal_write_u64_le,
   Gfun(External (EF_external "ssz_internal_write_u64_le"
                   (mksignature (AST.Xptr :: AST.Xlong :: nil) AST.Xvoid
                     cc_default)) ((tptr tuchar) :: tulong :: nil) tvoid
     cc_default)) ::
 (_ssz_internal_add_overflow_size, Gfun(Internal f_ssz_internal_add_overflow_size)) ::
 (_ssz_internal_mul_overflow_size, Gfun(Internal f_ssz_internal_mul_overflow_size)) ::
 (_ssz_internal_u64_to_size, Gfun(Internal f_ssz_internal_u64_to_size)) ::
 (_ssz_internal_bits_to_bytes, Gfun(Internal f_ssz_internal_bits_to_bytes)) ::
 (_ssz_internal_selector_allowed, Gfun(Internal f_ssz_internal_selector_allowed)) ::
 (_ssz_internal_validate_compatible_union_schema, Gfun(Internal f_ssz_internal_validate_compatible_union_schema)) ::
 (_ssz_internal_prepare_output, Gfun(Internal f_ssz_internal_prepare_output)) ::
 (_ssz_serialize_uint8, Gfun(Internal f_ssz_serialize_uint8)) ::
 (_ssz_serialize_uint16, Gfun(Internal f_ssz_serialize_uint16)) ::
 (_ssz_serialize_uint32, Gfun(Internal f_ssz_serialize_uint32)) ::
 (_ssz_serialize_uint64, Gfun(Internal f_ssz_serialize_uint64)) ::
 (_ssz_serialize_uint128, Gfun(Internal f_ssz_serialize_uint128)) ::
 (_ssz_serialize_uint256, Gfun(Internal f_ssz_serialize_uint256)) ::
 (_ssz_serialize_boolean, Gfun(Internal f_ssz_serialize_boolean)) ::
 (_ssz_serialize_bitvector, Gfun(Internal f_ssz_serialize_bitvector)) ::
 (_ssz_serialize_bitlist, Gfun(Internal f_ssz_serialize_bitlist)) ::
 (_ssz_serialize_vector_fixed, Gfun(Internal f_ssz_serialize_vector_fixed)) ::
 (_ssz_serialize_vector_variable, Gfun(Internal f_ssz_serialize_vector_variable)) ::
 (_ssz_serialize_list_fixed, Gfun(Internal f_ssz_serialize_list_fixed)) ::
 (_ssz_serialize_list_variable, Gfun(Internal f_ssz_serialize_list_variable)) ::
 (_ssz_serialize_container, Gfun(Internal f_ssz_serialize_container)) ::
 (_ssz_serialize_union, Gfun(Internal f_ssz_serialize_union)) ::
 (_ssz_serialize_compatible_union, Gfun(Internal f_ssz_serialize_compatible_union)) ::
 nil).

Definition public_idents : list ident :=
(_ssz_serialize_compatible_union :: _ssz_serialize_union ::
 _ssz_serialize_container :: _ssz_serialize_list_variable ::
 _ssz_serialize_list_fixed :: _ssz_serialize_vector_variable ::
 _ssz_serialize_vector_fixed :: _ssz_serialize_bitlist ::
 _ssz_serialize_bitvector :: _ssz_serialize_boolean ::
 _ssz_serialize_uint256 :: _ssz_serialize_uint128 :: _ssz_serialize_uint64 ::
 _ssz_serialize_uint32 :: _ssz_serialize_uint16 :: _ssz_serialize_uint8 ::
 _ssz_internal_write_u64_le :: _ssz_internal_write_u32_le ::
 _ssz_internal_write_u16_le :: _memset :: _memcpy :: ___builtin_debug ::
 ___builtin_fmin :: ___builtin_fmax :: ___builtin_fnmsub ::
 ___builtin_fnmadd :: ___builtin_fmsub :: ___builtin_fmadd ::
 ___builtin_clsll :: ___builtin_clsl :: ___builtin_cls ::
 ___builtin_expect :: ___builtin_unreachable :: ___builtin_va_end ::
 ___builtin_va_copy :: ___builtin_va_arg :: ___builtin_va_start ::
 ___builtin_membar :: ___builtin_annot_intval :: ___builtin_annot ::
 ___builtin_sel :: ___builtin_memcpy_aligned :: ___builtin_sqrt ::
 ___builtin_fsqrt :: ___builtin_fabsf :: ___builtin_fabs ::
 ___builtin_ctzll :: ___builtin_ctzl :: ___builtin_ctz :: ___builtin_clzll ::
 ___builtin_clzl :: ___builtin_clz :: ___builtin_bswap16 ::
 ___builtin_bswap32 :: ___builtin_bswap :: ___builtin_bswap64 ::
 ___compcert_i64_umulh :: ___compcert_i64_smulh :: ___compcert_i64_sar ::
 ___compcert_i64_shr :: ___compcert_i64_shl :: ___compcert_i64_umod ::
 ___compcert_i64_smod :: ___compcert_i64_udiv :: ___compcert_i64_sdiv ::
 ___compcert_i64_utof :: ___compcert_i64_stof :: ___compcert_i64_utod ::
 ___compcert_i64_stod :: ___compcert_i64_dtou :: ___compcert_i64_dtos ::
 ___compcert_va_composite :: ___compcert_va_float64 ::
 ___compcert_va_int64 :: ___compcert_va_int32 :: nil).

Definition prog : Clight.program := 
  mkprogram composites global_definitions public_idents _main Logic.I.


