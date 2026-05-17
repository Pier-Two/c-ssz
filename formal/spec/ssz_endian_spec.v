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

Definition Gprog : funspecs :=
  ltac:(with_library prog
    [ssz_internal_write_u16_le_spec; ssz_internal_read_u16_le_spec]).
