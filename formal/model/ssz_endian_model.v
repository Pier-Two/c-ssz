From Stdlib Require Import ZArith List Lia.
From compcert Require Import Integers.

Import ListNotations.
Local Open Scope Z_scope.

Definition byte_modulus : Z := 2 ^ 8.
Definition uint16_modulus : Z := 2 ^ 16.
Definition uint16_max : Z := uint16_modulus - 1.

Definition ssz_u16_le_byte0 (value : Z) : byte :=
  Byte.repr (value mod byte_modulus).

Definition ssz_u16_le_byte1 (value : Z) : byte :=
  Byte.repr ((value / byte_modulus) mod byte_modulus).

Definition ssz_u16_le_bytes (value : Z) : list byte :=
  [ssz_u16_le_byte0 value; ssz_u16_le_byte1 value].

Definition ssz_u16_le_decode (bytes : list byte) : Z :=
  match bytes with
  | [byte0; byte1] =>
      Byte.unsigned byte0 + byte_modulus * Byte.unsigned byte1
  | _ => 0
  end.

Lemma ssz_u16_le_bytes_length :
  forall value, Zlength (ssz_u16_le_bytes value) = 2.
Proof.
  intros; reflexivity.
Qed.

Lemma uint16_range_byte0 :
  forall value, 0 <= value <= uint16_max ->
    value mod byte_modulus = Z.land value 255.
Proof.
  intros value _.
  unfold byte_modulus.
  change 255 with (Z.ones 8).
  rewrite Z.land_ones by lia.
  reflexivity.
Qed.

Lemma ssz_u16_le_decode_range :
  forall byte0 byte1,
    0 <= ssz_u16_le_decode [byte0; byte1] <= uint16_max.
Proof.
  intros byte0 byte1.
  unfold ssz_u16_le_decode, byte_modulus, uint16_max, uint16_modulus.
  change (2 ^ 8) with 256.
  change (2 ^ 16) with 65536.
  pose proof (Byte.unsigned_range_2 byte0).
  pose proof (Byte.unsigned_range_2 byte1).
  change Byte.max_unsigned with 255 in *.
  lia.
Qed.

Lemma ssz_u16_le_decode_encode_roundtrip :
  forall value,
    0 <= value <= uint16_max ->
    ssz_u16_le_decode (ssz_u16_le_bytes value) = value.
Proof.
  intros value Hrange.
  unfold ssz_u16_le_decode, ssz_u16_le_bytes,
    ssz_u16_le_byte0, ssz_u16_le_byte1.
  assert (Hlow : 0 <= value mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound value 256 ltac:(lia)).
    lia.
  }
  assert (Hhigh : 0 <= (value / byte_modulus) mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound (value / 256) 256 ltac:(lia)).
    lia.
  }
  rewrite Byte.unsigned_repr by exact Hlow.
  rewrite Byte.unsigned_repr by exact Hhigh.
  unfold byte_modulus.
  change (2 ^ 8) with 256.
  assert (Hdiv_range : 0 <= value / 256 < 256).
  {
    unfold uint16_max, uint16_modulus in Hrange.
    change (2 ^ 16) with 65536 in Hrange.
    pose proof (Z.div_pos value 256 ltac:(lia) ltac:(lia)).
    assert (value / 256 < 256).
    {
      apply Z.div_lt_upper_bound; lia.
    }
    lia.
  }
  replace ((value / 256) mod 256) with (value / 256)
    by (rewrite Z.mod_small; lia).
  pose proof (Z.div_mod value 256 ltac:(lia)).
  lia.
Qed.

Lemma ssz_u16_le_bytes_injective :
  forall value1 value2,
    0 <= value1 <= uint16_max ->
    0 <= value2 <= uint16_max ->
    value1 <> value2 ->
    ssz_u16_le_bytes value1 <> ssz_u16_le_bytes value2.
Proof.
  intros value1 value2 Hrange1 Hrange2 Hneq Hbytes.
  apply Hneq.
  pose proof (f_equal ssz_u16_le_decode Hbytes) as Hdecoded.
  rewrite !ssz_u16_le_decode_encode_roundtrip in Hdecoded by assumption.
  exact Hdecoded.
Qed.

Lemma uint16_range_byte1 :
  forall value, 0 <= value <= uint16_max ->
    (value / byte_modulus) mod byte_modulus = Z.land (Z.shiftr value 8) 255.
Proof.
  intros value Hrange.
  unfold byte_modulus.
  rewrite Z.shiftr_div_pow2 by lia.
  change 255 with (Z.ones 8).
  rewrite Z.land_ones by lia.
  reflexivity.
Qed.
