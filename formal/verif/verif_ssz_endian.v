From Stdlib Require Import ZArith List Lia.
From compcert Require Import Integers.
Require Import VST.floyd.proofauto.
Require Import SSZFormal.clight.ssz_endian.
Require Import SSZFormal.model.ssz_endian_model.
Require Import SSZFormal.spec.ssz_endian_spec.

Import ListNotations.
Local Open Scope Z_scope.

Lemma write_u16_low_byte_value :
  forall value,
    0 <= value <= uint16_max ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.and (Int.repr value) (Int.repr 255)))) =
    Vubyte (ssz_u16_le_byte0 value).
Proof.
  intros value Hrange.
  assert (Hbyte : 0 <= value mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound value 256 ltac:(lia)).
    lia.
  }
  unfold Vubyte, ssz_u16_le_byte0.
  rewrite Byte.unsigned_repr by exact Hbyte.
  f_equal.
  rewrite and_repr.
  rewrite <- (uint16_range_byte0 value Hrange).
  rewrite Int.zero_ext_idem by lia.
  apply zero_ext_inrange.
  rewrite Int.unsigned_repr.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (two_p 8) with 256.
    pose proof (Z.mod_pos_bound value 256 ltac:(lia)).
    lia.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change Int.max_unsigned with 4294967295.
    pose proof (Z.mod_pos_bound value 256 ltac:(lia)).
    lia.
Qed.

Lemma write_u16_high_byte_value :
  forall value,
    0 <= value <= uint16_max ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.and (Int.shr (Int.repr value) (Int.repr 8))
               (Int.repr 255)))) =
    Vubyte (ssz_u16_le_byte1 value).
Proof.
  intros value Hrange.
  assert (Hbyte :
    0 <= (value / byte_modulus) mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound (value / 256) 256 ltac:(lia)).
    lia.
  }
  unfold Vubyte, ssz_u16_le_byte1.
  rewrite Byte.unsigned_repr by exact Hbyte.
  f_equal.
  rewrite Int.zero_ext_idem by lia.
  rewrite Int.shr_div_two_p.
  rewrite Int.signed_repr.
  2: {
    unfold uint16_max, uint16_modulus in Hrange.
    change (2 ^ 16) with 65536 in Hrange.
    change Int.max_signed with 2147483647.
    change Int.min_signed with (-2147483648).
    lia.
  }
  rewrite Int.unsigned_repr by (change Int.max_unsigned with 4294967295; lia).
  change (two_p 8) with 256.
  rewrite and_repr.
  replace (Z.land (value / 256) 255)
    with ((value / byte_modulus) mod byte_modulus).
  2: {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change 255 with (Z.ones 8).
    rewrite Z.land_ones by lia.
    reflexivity.
  }
  apply zero_ext_inrange.
  rewrite Int.unsigned_repr.
  - change (two_p 8) with 256.
    change Byte.max_unsigned with 255 in Hbyte.
    lia.
  - change Int.max_unsigned with 4294967295.
    change Byte.max_unsigned with 255 in Hbyte.
    lia.
Qed.

Lemma read_u16_return_value :
  forall byte0 byte1,
    Vint
      (Int.zero_ext 16
         (Int.or (Int.zero_ext 16 (Int.repr (Byte.unsigned byte0)))
            (Int.zero_ext 16
               (Int.shl (Int.zero_ext 16 (Int.repr (Byte.unsigned byte1)))
                  (Int.repr 8))))) =
    Vint (Int.repr (ssz_u16_le_decode [byte0; byte1])).
Proof.
  intros byte0 byte1.
  f_equal.
  pose proof (Byte.unsigned_range_2 byte0) as Hbyte0.
  pose proof (Byte.unsigned_range_2 byte1) as Hbyte1.
  change Byte.max_unsigned with 255 in *.
  assert (Hlow16 :
    Int.zero_ext 16 (Int.repr (Byte.unsigned byte0)) =
    Int.repr (Byte.unsigned byte0)).
  {
    apply zero_ext_inrange.
    rewrite Int.unsigned_repr.
    - change (two_p 16) with 65536; lia.
    - change Int.max_unsigned with 4294967295; lia.
  }
  assert (Hbyte1_16 :
    Int.zero_ext 16 (Int.repr (Byte.unsigned byte1)) =
    Int.repr (Byte.unsigned byte1)).
  {
    apply zero_ext_inrange.
    rewrite Int.unsigned_repr.
    - change (two_p 16) with 65536; lia.
    - change Int.max_unsigned with 4294967295; lia.
  }
  assert (Hhigh16 :
    Int.zero_ext 16
      (Int.shl (Int.repr (Byte.unsigned byte1)) (Int.repr 8)) =
    Int.shl (Int.repr (Byte.unsigned byte1)) (Int.repr 8)).
  {
    apply zero_ext_inrange.
    rewrite Int.shl_mul_two_p.
    rewrite Int.unsigned_repr by
      (change Int.max_unsigned with 4294967295; lia).
    change (two_p 8) with 256.
    rewrite Int.mul_signed.
    rewrite !Int.signed_repr by
      (change Int.max_signed with 2147483647;
       change Int.min_signed with (-2147483648);
       lia).
    rewrite Int.unsigned_repr.
    - change (two_p 16) with 65536; lia.
    - change Int.max_unsigned with 4294967295; lia.
  }
  unfold ssz_u16_le_decode, byte_modulus.
  change (2 ^ 8) with 256.
  rewrite Hlow16, Hbyte1_16, Hhigh16.
  rewrite Int.or_commut.
  rewrite Int.shifted_or_is_add.
  - change (two_p 8) with 256.
    rewrite Int.unsigned_repr by
      (change Int.max_unsigned with 4294967295; lia).
    rewrite Int.unsigned_repr by
      (change Int.max_unsigned with 4294967295; lia).
    replace (Byte.unsigned byte1 * 256 + Byte.unsigned byte0)
      with (Byte.unsigned byte0 + 256 * Byte.unsigned byte1) by lia.
    apply zero_ext_inrange.
    rewrite Int.unsigned_repr.
    + change (two_p 16) with 65536; lia.
    + change Int.max_unsigned with 4294967295; lia.
  - change Int.zwordsize with 32; lia.
  - rewrite Int.unsigned_repr by
      (change Int.max_unsigned with 4294967295; lia).
    change (two_p 8) with 256; lia.
Qed.

Lemma body_ssz_internal_write_u16_le :
  semax_body Vprog Gprog
    f_ssz_internal_write_u16_le ssz_internal_write_u16_le_spec.
Proof.
  start_function.
  unfold t_ssz_u16_bytes.
  rewrite data_at__tarray.
  change (Zrepeat (default_val tuchar) 2) with ([Vundef; Vundef]).
  forward.
  forward.
  unfold t_ssz_u16_bytes, ssz_u16_le_bytes,
    ssz_u16_le_byte0, ssz_u16_le_byte1.
  change
    (upd_Znth 1
       (upd_Znth 0 [Vundef; Vundef]
          (Vint
             (Int.zero_ext 8
                (Int.zero_ext 8
                   (Int.and (Int.repr value) (Int.repr 255))))))
       (Vint
          (Int.zero_ext 8
             (Int.zero_ext 8
                (Int.and (Int.shr (Int.repr value) (Int.repr 8))
                   (Int.repr 255)))))) with
    ([Vint
        (Int.zero_ext 8
           (Int.zero_ext 8 (Int.and (Int.repr value) (Int.repr 255))));
      Vint
        (Int.zero_ext 8
           (Int.zero_ext 8
              (Int.and (Int.shr (Int.repr value) (Int.repr 8))
                 (Int.repr 255))))]).
  rewrite (write_u16_low_byte_value value H).
  rewrite (write_u16_high_byte_value value H).
  entailer!.
Qed.

Lemma body_ssz_internal_read_u16_le :
  semax_body Vprog Gprog
    f_ssz_internal_read_u16_le ssz_internal_read_u16_le_spec.
Proof.
  start_function.
  forward.
  - change (Znth 0 (map Vubyte [byte0; byte1])) with (Vubyte byte0).
    unfold Vubyte.
    entailer!.
    pose proof (Byte.unsigned_range_2 byte0).
    lia.
  - forward.
    + change (Znth 1 (map Vubyte [byte0; byte1])) with (Vubyte byte1).
      unfold Vubyte.
      entailer!.
      pose proof (Byte.unsigned_range_2 byte1).
      lia.
    + forward.
      entailer!.
      change (Znth 0 [byte0; byte1]) with byte0.
      change (Znth 1 [byte0; byte1]) with byte1.
      unfold Vubyte.
      simpl.
      apply read_u16_return_value.
Qed.
