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

Lemma zero_ext_u8_repr :
  forall byte, Int.zero_ext 8 (Int.repr (Byte.unsigned byte)) =
    Int.repr (Byte.unsigned byte).
Proof.
  intros byte.
  pose proof (Byte.unsigned_range_2 byte).
  change Byte.max_unsigned with 255 in *.
  apply zero_ext_inrange.
  rewrite Int.unsigned_repr.
  - change (two_p 8) with 256; lia.
  - change Int.max_unsigned with 4294967295; lia.
Qed.

Lemma write_u32_low_byte_value :
  forall value,
    0 <= value <= uint32_max ->
    Vint
      (Int.zero_ext 8 (Int.and (Int.repr value) (Int.repr 255))) =
    Vubyte (ssz_u32_le_byte0 value).
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
  unfold Vubyte, ssz_u32_le_byte0.
  rewrite Byte.unsigned_repr by exact Hbyte.
  f_equal.
  rewrite and_repr.
  replace (Z.land value 255) with (value mod byte_modulus).
  2: {
    unfold byte_modulus.
    change 255 with (Z.ones 8).
    rewrite Z.land_ones by lia.
    reflexivity.
  }
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

Lemma write_u32_shifted_byte_value :
  forall value shift divisor byte_value,
    0 <= value <= uint32_max ->
    divisor = two_p shift ->
    0 <= shift < Int.zwordsize ->
    byte_value = Byte.repr ((value / divisor) mod byte_modulus) ->
    Vint
      (Int.zero_ext 8
         (Int.and (Int.shru (Int.repr value) (Int.repr shift))
            (Int.repr 255))) =
    Vubyte byte_value.
Proof.
  intros value shift divisor byte_value Hrange Hdivisor Hshift Hbyte_value.
  subst byte_value.
  change Int.zwordsize with 32 in Hshift.
  assert (Hbyte :
    0 <= (value / divisor) mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound (value / divisor) 256 ltac:(lia)).
    lia.
  }
  unfold Vubyte.
  rewrite Byte.unsigned_repr by exact Hbyte.
  f_equal.
  rewrite Int.shru_div_two_p.
  rewrite Int.unsigned_repr by
    (unfold uint32_max, uint32_modulus in Hrange;
     change (2 ^ 32) with 4294967296 in Hrange;
     change Int.max_unsigned with 4294967295; lia).
  rewrite Int.unsigned_repr by
    (change Int.max_unsigned with 4294967295; lia).
  replace (two_p shift) with divisor by lia.
  rewrite and_repr.
  replace (Z.land (value / divisor) 255)
    with ((value / divisor) mod byte_modulus).
  2: {
    unfold byte_modulus.
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

Lemma write_u32_byte1_value :
  forall value,
    0 <= value <= uint32_max ->
    Vint
      (Int.zero_ext 8
         (Int.and (Int.shru (Int.repr value) (Int.repr 8))
            (Int.repr 255))) =
    Vubyte (ssz_u32_le_byte1 value).
Proof.
  intros value Hrange.
  unfold ssz_u32_le_byte1.
  eapply write_u32_shifted_byte_value; try exact Hrange.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (two_p 8) with 256.
    reflexivity.
  - change Int.zwordsize with 32; lia.
  - reflexivity.
Qed.

Lemma write_u32_byte2_value :
  forall value,
    0 <= value <= uint32_max ->
    Vint
      (Int.zero_ext 8
         (Int.and (Int.shru (Int.repr value) (Int.repr 16))
            (Int.repr 255))) =
    Vubyte (ssz_u32_le_byte2 value).
Proof.
  intros value Hrange.
  unfold ssz_u32_le_byte2.
  eapply write_u32_shifted_byte_value; try exact Hrange.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 2) with 65536.
    change (two_p 16) with 65536.
    reflexivity.
  - change Int.zwordsize with 32; lia.
  - reflexivity.
Qed.

Lemma write_u32_byte3_value :
  forall value,
    0 <= value <= uint32_max ->
    Vint
      (Int.zero_ext 8
         (Int.and (Int.shru (Int.repr value) (Int.repr 24))
            (Int.repr 255))) =
    Vubyte (ssz_u32_le_byte3 value).
Proof.
  intros value Hrange.
  unfold ssz_u32_le_byte3.
  eapply write_u32_shifted_byte_value; try exact Hrange.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 3) with 16777216.
    change (two_p 24) with 16777216.
    reflexivity.
  - change Int.zwordsize with 32; lia.
  - reflexivity.
Qed.

Lemma read_u32_return_value :
  forall byte0 byte1 byte2 byte3,
    Vint
      (Int.or
         (Int.or
            (Int.or (Int.zero_ext 8 (Int.repr (Byte.unsigned byte0)))
               (Int.shl (Int.zero_ext 8 (Int.repr (Byte.unsigned byte1)))
                  (Int.repr 8)))
            (Int.shl (Int.zero_ext 8 (Int.repr (Byte.unsigned byte2)))
               (Int.repr 16)))
         (Int.shl (Int.zero_ext 8 (Int.repr (Byte.unsigned byte3)))
            (Int.repr 24))) =
    Vint (Int.repr (ssz_u32_le_decode [byte0; byte1; byte2; byte3])).
Proof.
  intros byte0 byte1 byte2 byte3.
  f_equal.
  pose proof (Byte.unsigned_range_2 byte0) as Hbyte0.
  pose proof (Byte.unsigned_range_2 byte1) as Hbyte1.
  pose proof (Byte.unsigned_range_2 byte2) as Hbyte2.
  pose proof (Byte.unsigned_range_2 byte3) as Hbyte3.
  change Byte.max_unsigned with 255 in *.
  rewrite !zero_ext_u8_repr.
  unfold ssz_u32_le_decode, byte_modulus.
  change (2 ^ 8) with 256.
  change (256 ^ 2) with 65536.
  change (256 ^ 3) with 16777216.
  replace (Int.or (Int.repr (Byte.unsigned byte0))
             (Int.shl (Int.repr (Byte.unsigned byte1)) (Int.repr 8)))
    with (Int.repr (Byte.unsigned byte0 + 256 * Byte.unsigned byte1)).
  2: {
    rewrite Int.or_commut.
    rewrite Int.shifted_or_is_add.
    - change (two_p 8) with 256.
      rewrite !Int.unsigned_repr by
        (change Int.max_unsigned with 4294967295; lia).
      f_equal; lia.
    - change Int.zwordsize with 32; lia.
    - rewrite Int.unsigned_repr by
        (change Int.max_unsigned with 4294967295; lia).
      change (two_p 8) with 256; lia.
  }
  replace (Int.or
             (Int.repr
                (Byte.unsigned byte0 + 256 * Byte.unsigned byte1))
             (Int.shl (Int.repr (Byte.unsigned byte2)) (Int.repr 16)))
    with (Int.repr
            (Byte.unsigned byte0 + 256 * Byte.unsigned byte1 +
             65536 * Byte.unsigned byte2)).
  2: {
    rewrite Int.or_commut.
    rewrite Int.shifted_or_is_add.
    - change (two_p 16) with 65536.
      rewrite !Int.unsigned_repr by
        (change Int.max_unsigned with 4294967295; lia).
      f_equal; lia.
    - change Int.zwordsize with 32; lia.
    - rewrite Int.unsigned_repr by
        (change Int.max_unsigned with 4294967295; lia).
      change (two_p 16) with 65536; lia.
  }
  replace (Int.or
             (Int.repr
                (Byte.unsigned byte0 + 256 * Byte.unsigned byte1 +
                 65536 * Byte.unsigned byte2))
             (Int.shl (Int.repr (Byte.unsigned byte3)) (Int.repr 24)))
    with (Int.repr
            (Byte.unsigned byte0 + 256 * Byte.unsigned byte1 +
             65536 * Byte.unsigned byte2 +
             16777216 * Byte.unsigned byte3)).
  2: {
    rewrite Int.or_commut.
    rewrite Int.shifted_or_is_add.
    - change (two_p 24) with 16777216.
      rewrite !Int.unsigned_repr by
        (change Int.max_unsigned with 4294967295; lia).
      f_equal; lia.
    - change Int.zwordsize with 32; lia.
    - rewrite Int.unsigned_repr by
        (change Int.max_unsigned with 4294967295; lia).
      change (two_p 24) with 16777216; lia.
  }
  f_equal; lia.
Qed.

Lemma int64_shl'_repr :
  forall value shift,
    0 <= shift < Int64.zwordsize ->
    Int64.shl' value (Int.repr shift) =
    Int64.shl value (Int64.repr shift).
Proof.
  intros value shift Hshift.
  unfold Int64.shl', Int64.shl.
  rewrite Int.unsigned_repr by
    (change Int64.zwordsize with 64 in Hshift;
     change Int.max_unsigned with 4294967295; lia).
  rewrite Int64.unsigned_repr by
    (change Int64.zwordsize with 64 in Hshift;
     change Int64.max_unsigned with 18446744073709551615; lia).
  reflexivity.
Qed.

Lemma int64_shru'_repr :
  forall value shift,
    0 <= shift < Int64.zwordsize ->
    Int64.shru' value (Int.repr shift) =
    Int64.shru value (Int64.repr shift).
Proof.
  intros value shift Hshift.
  unfold Int64.shru', Int64.shru.
  rewrite Int.unsigned_repr by
    (change Int64.zwordsize with 64 in Hshift;
     change Int.max_unsigned with 4294967295; lia).
  rewrite Int64.unsigned_repr by
    (change Int64.zwordsize with 64 in Hshift;
     change Int64.max_unsigned with 18446744073709551615; lia).
  reflexivity.
Qed.

Lemma write_u64_low_byte_value :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int64.loword
            (Int64.and (Int64.repr value) (Int64.repr 255)))) =
    Vubyte (ssz_u64_le_byte0 value).
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
  unfold Vubyte, ssz_u64_le_byte0.
  rewrite Byte.unsigned_repr by exact Hbyte.
  f_equal.
  unfold Int64.loword.
  change (Int64.repr 255) with
    (Int64.repr (two_p 8 - 1)).
  rewrite <- Int64.zero_ext_and by lia.
  rewrite Int64.zero_ext_mod by
    (change Int64.zwordsize with 64; lia).
  rewrite Int64.unsigned_repr by
    (unfold uint64_max, uint64_modulus in Hrange;
     change (2 ^ 64) with 18446744073709551616 in Hrange;
     change Int64.max_unsigned with 18446744073709551615; lia).
  change (two_p 8) with 256.
  apply zero_ext_inrange.
  rewrite Int.unsigned_repr.
  - unfold byte_modulus in *.
    change (2 ^ 8) with 256 in *.
    change (two_p 8) with 256.
    change Byte.max_unsigned with 255 in *.
    lia.
  - unfold byte_modulus in *.
    change (2 ^ 8) with 256 in *.
    change Byte.max_unsigned with 255 in *.
    change Int.max_unsigned with 4294967295.
    lia.
Qed.

Lemma write_u64_shifted_byte_value :
  forall value shift divisor byte_value,
    0 <= value <= uint64_max ->
    divisor = two_p shift ->
    0 <= shift < Int64.zwordsize ->
    byte_value = Byte.repr ((value / divisor) mod byte_modulus) ->
    Vint
      (Int.zero_ext 8
         (Int64.loword
            (Int64.and
               (Int64.shru' (Int64.repr value) (Int.repr shift))
               (Int64.repr 255)))) =
    Vubyte byte_value.
Proof.
  intros value shift divisor byte_value Hrange Hdivisor Hshift Hbyte_value.
  subst byte_value.
  change Int64.zwordsize with 64 in Hshift.
  assert (Hdivisor_pos : 0 < divisor) by
    (subst divisor; pose proof (two_p_gt_ZERO shift); lia).
  assert (Hbyte :
    0 <= (value / divisor) mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound (value / divisor) 256 ltac:(lia)).
    lia.
  }
  assert (Hquot :
    0 <= value / divisor <= Int64.max_unsigned).
  {
    split.
    - apply Z.div_pos; lia.
    - apply Z.div_le_upper_bound; [lia|].
      unfold uint64_max, uint64_modulus in Hrange.
      change (2 ^ 64) with 18446744073709551616 in Hrange.
      change Int64.max_unsigned with 18446744073709551615.
      nia.
  }
  unfold Vubyte.
  rewrite Byte.unsigned_repr by exact Hbyte.
  f_equal.
  unfold Int64.loword.
  rewrite int64_shru'_repr by
    (change Int64.zwordsize with 64; lia).
  rewrite Int64.shru_div_two_p.
  rewrite Int64.unsigned_repr by
    (unfold uint64_max, uint64_modulus in Hrange;
     change (2 ^ 64) with 18446744073709551616 in Hrange;
     change Int64.max_unsigned with 18446744073709551615; lia).
  rewrite Int64.unsigned_repr by
    (change Int64.max_unsigned with 18446744073709551615; lia).
  replace (two_p shift) with divisor by lia.
  change (Int64.repr 255) with
    (Int64.repr (two_p 8 - 1)).
  rewrite <- Int64.zero_ext_and by lia.
  rewrite Int64.zero_ext_mod by
    (change Int64.zwordsize with 64; lia).
  rewrite Int64.unsigned_repr by exact Hquot.
  change (two_p 8) with 256.
  apply zero_ext_inrange.
  rewrite Int.unsigned_repr.
  - change (two_p 8) with 256.
    change Byte.max_unsigned with 255 in *.
    unfold byte_modulus in *.
    change (2 ^ 8) with 256 in *.
    lia.
  - change Int.max_unsigned with 4294967295.
    change Byte.max_unsigned with 255 in *.
    unfold byte_modulus in *.
    change (2 ^ 8) with 256 in *.
    lia.
Qed.

Lemma write_u64_byte1_value :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int64.loword
            (Int64.and
               (Int64.shru' (Int64.repr value) (Int.repr 8))
               (Int64.repr 255)))) =
    Vubyte (ssz_u64_le_byte1 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte1.
  eapply write_u64_shifted_byte_value; try exact Hrange.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (two_p 8) with 256.
    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_byte2_value :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int64.loword
            (Int64.and
               (Int64.shru' (Int64.repr value) (Int.repr 16))
               (Int64.repr 255)))) =
    Vubyte (ssz_u64_le_byte2 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte2.
  eapply write_u64_shifted_byte_value; try exact Hrange.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 2) with 65536.
    change (two_p 16) with 65536.
    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_byte3_value :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int64.loword
            (Int64.and
               (Int64.shru' (Int64.repr value) (Int.repr 24))
               (Int64.repr 255)))) =
    Vubyte (ssz_u64_le_byte3 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte3.
  eapply write_u64_shifted_byte_value; try exact Hrange.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 3) with 16777216.
    change (two_p 24) with 16777216.
    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_byte4_value :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int64.loword
            (Int64.and
               (Int64.shru' (Int64.repr value) (Int.repr 32))
               (Int64.repr 255)))) =
    Vubyte (ssz_u64_le_byte4 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte4.
  eapply write_u64_shifted_byte_value; try exact Hrange.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 4) with 4294967296.
    change (two_p 32) with 4294967296.
    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_byte5_value :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int64.loword
            (Int64.and
               (Int64.shru' (Int64.repr value) (Int.repr 40))
               (Int64.repr 255)))) =
    Vubyte (ssz_u64_le_byte5 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte5.
  eapply write_u64_shifted_byte_value; try exact Hrange.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 5) with 1099511627776.
    change (two_p 40) with 1099511627776.
    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_byte6_value :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int64.loword
            (Int64.and
               (Int64.shru' (Int64.repr value) (Int.repr 48))
               (Int64.repr 255)))) =
    Vubyte (ssz_u64_le_byte6 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte6.
  eapply write_u64_shifted_byte_value; try exact Hrange.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 6) with 281474976710656.
    change (two_p 48) with 281474976710656.
    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_byte7_value :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int64.loword
            (Int64.and
               (Int64.shru' (Int64.repr value) (Int.repr 56))
               (Int64.repr 255)))) =
    Vubyte (ssz_u64_le_byte7 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte7.
  eapply write_u64_shifted_byte_value; try exact Hrange.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 7) with 72057594037927936.
    change (two_p 56) with 72057594037927936.
    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_low_byte_value_norm :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.repr
               (Int64.Z_mod_modulus
                  (Z.land (Int64.Z_mod_modulus value) 255))))) =
    Vubyte (ssz_u64_le_byte0 value).
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
  replace (Int64.Z_mod_modulus (Z.land (Int64.Z_mod_modulus value) 255))
    with (value mod byte_modulus).
  2: {
    rewrite !Int64.Z_mod_modulus_eq.
    change Int64.modulus with 18446744073709551616.
    assert (Hvalue_modulus : 0 <= value < 18446744073709551616).
    {
      unfold uint64_max, uint64_modulus in Hrange.
      change (2 ^ 64) with 18446744073709551616 in Hrange.
      lia.
    }
    rewrite (Z.mod_small value 18446744073709551616 Hvalue_modulus).
    change 255 with (Z.ones 8).
    rewrite Z.land_ones by lia.
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    pose proof (Z.mod_pos_bound value 256 ltac:(lia)) as Hvalue_byte.
    rewrite (Z.mod_small (value mod 256) 18446744073709551616)
      by lia.
    reflexivity.
  }
  rewrite Int.zero_ext_idem by lia.
  unfold Vubyte, ssz_u64_le_byte0.
  rewrite Byte.unsigned_repr by exact Hbyte.
  f_equal.
  apply zero_ext_inrange.
  rewrite Int.unsigned_repr.
  - unfold byte_modulus in *.
    change (2 ^ 8) with 256 in *.
    change (two_p 8) with 256.
    change Byte.max_unsigned with 255 in Hbyte.
    lia.
  - unfold byte_modulus in *.
    change (2 ^ 8) with 256 in *.
    change Byte.max_unsigned with 255 in Hbyte.
    change Int.max_unsigned with 4294967295.
    lia.
Qed.

Lemma write_u64_shifted_byte_value_norm :
  forall value shift divisor byte_value,
    0 <= value <= uint64_max ->
    divisor = 2 ^ shift ->
    0 <= shift < Int64.zwordsize ->
    byte_value = Byte.repr ((value / divisor) mod byte_modulus) ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.repr
               (Int64.Z_mod_modulus
                  (Z.land
                     (Int64.Z_mod_modulus
                        (Z.shiftr (Int64.Z_mod_modulus value) shift))
                     255))))) =
    Vubyte byte_value.
Proof.
  intros value shift divisor byte_value Hrange Hdivisor Hshift Hbyte_value.
  subst byte_value.
  change Int64.zwordsize with 64 in Hshift.
  assert (Hdivisor_pos : 0 < divisor) by
    (subst divisor; apply Z.pow_pos_nonneg; lia).
  assert (Hbyte :
    0 <= (value / divisor) mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound (value / divisor) 256 ltac:(lia)).
    lia.
  }
  assert (Hquot :
    0 <= value / divisor <= Int64.max_unsigned).
  {
    split.
    - apply Z.div_pos; lia.
    - apply Z.div_le_upper_bound; [lia|].
      unfold uint64_max, uint64_modulus in Hrange.
      change (2 ^ 64) with 18446744073709551616 in Hrange.
      change Int64.max_unsigned with 18446744073709551615.
      nia.
  }
  replace
    (Int64.Z_mod_modulus
       (Z.land
          (Int64.Z_mod_modulus
             (Z.shiftr (Int64.Z_mod_modulus value) shift)) 255))
    with ((value / divisor) mod byte_modulus).
  2: {
    rewrite !Int64.Z_mod_modulus_eq.
    change Int64.modulus with 18446744073709551616.
    assert (Hvalue_modulus : 0 <= value < 18446744073709551616).
    {
      unfold uint64_max, uint64_modulus in Hrange.
      change (2 ^ 64) with 18446744073709551616 in Hrange.
      lia.
    }
    rewrite (Z.mod_small value 18446744073709551616 Hvalue_modulus).
    rewrite Z.shiftr_div_pow2 by lia.
    replace (2 ^ shift) with divisor by lia.
    assert (Hquot_modulus :
      0 <= value / divisor < 18446744073709551616).
    {
      change Int64.max_unsigned with 18446744073709551615 in Hquot.
      lia.
    }
    rewrite (Z.mod_small (value / divisor) 18446744073709551616
      Hquot_modulus).
    change 255 with (Z.ones 8).
    rewrite Z.land_ones by lia.
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    pose proof (Z.mod_pos_bound (value / divisor) 256 ltac:(lia))
      as Hvalue_byte.
    rewrite (Z.mod_small ((value / divisor) mod 256)
      18446744073709551616) by lia.
    reflexivity.
  }
  rewrite Int.zero_ext_idem by lia.
  unfold Vubyte.
  rewrite Byte.unsigned_repr by exact Hbyte.
  f_equal.
  apply zero_ext_inrange.
  rewrite Int.unsigned_repr.
  - unfold byte_modulus in *.
    change (2 ^ 8) with 256 in *.
    change (two_p 8) with 256.
    change Byte.max_unsigned with 255 in Hbyte.
    lia.
  - unfold byte_modulus in *.
    change (2 ^ 8) with 256 in *.
    change Byte.max_unsigned with 255 in Hbyte.
    change Int.max_unsigned with 4294967295.
    lia.
Qed.

Lemma write_u64_byte1_value_norm :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.repr
               (Int64.Z_mod_modulus
                  (Z.land
                     (Int64.Z_mod_modulus
                        (Z.shiftr (Int64.Z_mod_modulus value) 8))
                     255))))) =
    Vubyte (ssz_u64_le_byte1 value).
Proof.
  intros value Hrange.
	  unfold ssz_u64_le_byte1.
	  eapply write_u64_shifted_byte_value_norm; try exact Hrange.
	  - unfold byte_modulus.
	    change (2 ^ 8) with 256.
	    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_byte2_value_norm :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.repr
               (Int64.Z_mod_modulus
                  (Z.land
                     (Int64.Z_mod_modulus
                        (Z.shiftr (Int64.Z_mod_modulus value) 16))
                     255))))) =
    Vubyte (ssz_u64_le_byte2 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte2.
  eapply write_u64_shifted_byte_value_norm; try exact Hrange.
	  - unfold byte_modulus.
	    change (2 ^ 8) with 256.
	    change (256 ^ 2) with 65536.
	    change (2 ^ 16) with 65536.
	    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_byte3_value_norm :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.repr
               (Int64.Z_mod_modulus
                  (Z.land
                     (Int64.Z_mod_modulus
                        (Z.shiftr (Int64.Z_mod_modulus value) 24))
                     255))))) =
    Vubyte (ssz_u64_le_byte3 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte3.
  eapply write_u64_shifted_byte_value_norm; try exact Hrange.
	  - unfold byte_modulus.
	    change (2 ^ 8) with 256.
	    change (256 ^ 3) with 16777216.
	    change (2 ^ 24) with 16777216.
	    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_byte4_value_norm :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.repr
               (Int64.Z_mod_modulus
                  (Z.land
                     (Int64.Z_mod_modulus
                        (Z.shiftr (Int64.Z_mod_modulus value) 32))
                     255))))) =
    Vubyte (ssz_u64_le_byte4 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte4.
  eapply write_u64_shifted_byte_value_norm; try exact Hrange.
	  - unfold byte_modulus.
	    change (2 ^ 8) with 256.
	    change (256 ^ 4) with 4294967296.
	    change (2 ^ 32) with 4294967296.
	    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_byte5_value_norm :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.repr
               (Int64.Z_mod_modulus
                  (Z.land
                     (Int64.Z_mod_modulus
                        (Z.shiftr (Int64.Z_mod_modulus value) 40))
                     255))))) =
    Vubyte (ssz_u64_le_byte5 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte5.
  eapply write_u64_shifted_byte_value_norm; try exact Hrange.
	  - unfold byte_modulus.
	    change (2 ^ 8) with 256.
	    change (256 ^ 5) with 1099511627776.
	    change (2 ^ 40) with 1099511627776.
	    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_byte6_value_norm :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.repr
               (Int64.Z_mod_modulus
                  (Z.land
                     (Int64.Z_mod_modulus
                        (Z.shiftr (Int64.Z_mod_modulus value) 48))
                     255))))) =
    Vubyte (ssz_u64_le_byte6 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte6.
  eapply write_u64_shifted_byte_value_norm; try exact Hrange.
	  - unfold byte_modulus.
	    change (2 ^ 8) with 256.
	    change (256 ^ 6) with 281474976710656.
	    change (2 ^ 48) with 281474976710656.
	    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_byte7_value_norm :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.repr
               (Int64.Z_mod_modulus
                  (Z.land
                     (Int64.Z_mod_modulus
                        (Z.shiftr (Int64.Z_mod_modulus value) 56))
                     255))))) =
    Vubyte (ssz_u64_le_byte7 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte7.
  eapply write_u64_shifted_byte_value_norm; try exact Hrange.
	  - unfold byte_modulus.
	    change (2 ^ 8) with 256.
	    change (256 ^ 7) with 72057594037927936.
	    change (2 ^ 56) with 72057594037927936.
	    reflexivity.
  - change Int64.zwordsize with 64; lia.
	  - reflexivity.
Qed.

Lemma write_u64_low_byte_value_store :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.repr
               (Int64.unsigned
                  (Int64.and (Int64.repr value)
                     (Int64.repr (Int.unsigned (Int.repr 255)))))))) =
    Vubyte (ssz_u64_le_byte0 value).
Proof.
  intros value Hrange.
  change (Int.unsigned (Int.repr 255)) with 255.
  rewrite Int.zero_ext_idem by lia.
  change (Int.repr (Int64.unsigned
    (Int64.and (Int64.repr value) (Int64.repr 255)))) with
    (Int64.loword (Int64.and (Int64.repr value) (Int64.repr 255))).
  apply write_u64_low_byte_value; exact Hrange.
Qed.

Lemma write_u64_shifted_byte_value_store :
  forall value shift divisor byte_value,
    0 <= value <= uint64_max ->
    divisor = two_p shift ->
    0 <= shift < Int64.zwordsize ->
    byte_value = Byte.repr ((value / divisor) mod byte_modulus) ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.repr
               (Int64.unsigned
                  (Int64.and
                     (Int64.shru (Int64.repr value)
                        (Int64.repr (Int.unsigned (Int.repr shift))))
                     (Int64.repr (Int.unsigned (Int.repr 255)))))))) =
    Vubyte byte_value.
Proof.
  intros value shift divisor byte_value Hrange Hdivisor Hshift Hbyte_value.
  change (Int.unsigned (Int.repr 255)) with 255.
  rewrite Int.unsigned_repr by
    (change Int64.zwordsize with 64 in Hshift;
     change Int.max_unsigned with 4294967295; lia).
  rewrite <- (int64_shru'_repr (Int64.repr value) shift) by exact Hshift.
  rewrite Int.zero_ext_idem by lia.
  change (Int.repr (Int64.unsigned
    (Int64.and (Int64.shru' (Int64.repr value) (Int.repr shift))
       (Int64.repr 255)))) with
    (Int64.loword
       (Int64.and (Int64.shru' (Int64.repr value) (Int.repr shift))
          (Int64.repr 255))).
  eapply write_u64_shifted_byte_value; eauto.
Qed.

Lemma write_u64_byte1_value_store :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.repr
               (Int64.unsigned
                  (Int64.and
                     (Int64.shru (Int64.repr value)
                        (Int64.repr (Int.unsigned (Int.repr 8))))
                     (Int64.repr (Int.unsigned (Int.repr 255)))))))) =
    Vubyte (ssz_u64_le_byte1 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte1.
  eapply write_u64_shifted_byte_value_store; try exact Hrange.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (two_p 8) with 256.
    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_byte2_value_store :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.repr
               (Int64.unsigned
                  (Int64.and
                     (Int64.shru (Int64.repr value)
                        (Int64.repr (Int.unsigned (Int.repr 16))))
                     (Int64.repr (Int.unsigned (Int.repr 255)))))))) =
    Vubyte (ssz_u64_le_byte2 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte2.
  eapply write_u64_shifted_byte_value_store; try exact Hrange.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 2) with 65536.
    change (two_p 16) with 65536.
    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_byte3_value_store :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.repr
               (Int64.unsigned
                  (Int64.and
                     (Int64.shru (Int64.repr value)
                        (Int64.repr (Int.unsigned (Int.repr 24))))
                     (Int64.repr (Int.unsigned (Int.repr 255)))))))) =
    Vubyte (ssz_u64_le_byte3 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte3.
  eapply write_u64_shifted_byte_value_store; try exact Hrange.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 3) with 16777216.
    change (two_p 24) with 16777216.
    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_byte4_value_store :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.repr
               (Int64.unsigned
                  (Int64.and
                     (Int64.shru (Int64.repr value)
                        (Int64.repr (Int.unsigned (Int.repr 32))))
                     (Int64.repr (Int.unsigned (Int.repr 255)))))))) =
    Vubyte (ssz_u64_le_byte4 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte4.
  eapply write_u64_shifted_byte_value_store; try exact Hrange.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 4) with 4294967296.
    change (two_p 32) with 4294967296.
    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_byte5_value_store :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.repr
               (Int64.unsigned
                  (Int64.and
                     (Int64.shru (Int64.repr value)
                        (Int64.repr (Int.unsigned (Int.repr 40))))
                     (Int64.repr (Int.unsigned (Int.repr 255)))))))) =
    Vubyte (ssz_u64_le_byte5 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte5.
  eapply write_u64_shifted_byte_value_store; try exact Hrange.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 5) with 1099511627776.
    change (two_p 40) with 1099511627776.
    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_byte6_value_store :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.repr
               (Int64.unsigned
                  (Int64.and
                     (Int64.shru (Int64.repr value)
                        (Int64.repr (Int.unsigned (Int.repr 48))))
                     (Int64.repr (Int.unsigned (Int.repr 255)))))))) =
    Vubyte (ssz_u64_le_byte6 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte6.
  eapply write_u64_shifted_byte_value_store; try exact Hrange.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 6) with 281474976710656.
    change (two_p 48) with 281474976710656.
    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma write_u64_byte7_value_store :
  forall value,
    0 <= value <= uint64_max ->
    Vint
      (Int.zero_ext 8
         (Int.zero_ext 8
            (Int.repr
               (Int64.unsigned
                  (Int64.and
                     (Int64.shru (Int64.repr value)
                        (Int64.repr (Int.unsigned (Int.repr 56))))
                     (Int64.repr (Int.unsigned (Int.repr 255)))))))) =
    Vubyte (ssz_u64_le_byte7 value).
Proof.
  intros value Hrange.
  unfold ssz_u64_le_byte7.
  eapply write_u64_shifted_byte_value_store; try exact Hrange.
  - unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 7) with 72057594037927936.
    change (two_p 56) with 72057594037927936.
    reflexivity.
  - change Int64.zwordsize with 64; lia.
  - reflexivity.
Qed.

Lemma int64_or_shifted_byte :
  forall low byte shift scale,
    0 <= low < scale ->
    0 <= low <= Int64.max_unsigned ->
    scale = two_p shift ->
    0 <= shift < Int64.zwordsize ->
    Int64.or (Int64.repr low)
      (Int64.shl' (Int64.repr (Byte.unsigned byte)) (Int.repr shift)) =
    Int64.repr (low + scale * Byte.unsigned byte).
Proof.
  intros low byte shift scale Hlow Hlow64 Hscale Hshift.
  pose proof (Byte.unsigned_range_2 byte) as Hbyte.
  change Byte.max_unsigned with 255 in Hbyte.
  rewrite int64_shl'_repr by exact Hshift.
  rewrite Int64.or_commut.
  rewrite Int64.shifted_or_is_add.
  - rewrite Int64.unsigned_repr by
      (change Int64.max_unsigned with 18446744073709551615; lia).
    rewrite Int64.unsigned_repr by exact Hlow64.
    f_equal; lia.
  - exact Hshift.
  - rewrite Int64.unsigned_repr by exact Hlow64.
    lia.
Qed.

Lemma read_u64_return_value :
  forall byte0 byte1 byte2 byte3 byte4 byte5 byte6 byte7,
    Vlong
      (Int64.or
         (Int64.or
            (Int64.or
               (Int64.or
                  (Int64.or
                     (Int64.or
                        (Int64.or
                           (Int64.repr (Byte.unsigned byte0))
                           (Int64.shl'
                              (Int64.repr (Byte.unsigned byte1))
                              (Int.repr 8)))
                        (Int64.shl' (Int64.repr (Byte.unsigned byte2))
                           (Int.repr 16)))
                     (Int64.shl' (Int64.repr (Byte.unsigned byte3))
                        (Int.repr 24)))
                  (Int64.shl' (Int64.repr (Byte.unsigned byte4))
                     (Int.repr 32)))
               (Int64.shl' (Int64.repr (Byte.unsigned byte5))
                  (Int.repr 40)))
            (Int64.shl' (Int64.repr (Byte.unsigned byte6))
               (Int.repr 48)))
         (Int64.shl' (Int64.repr (Byte.unsigned byte7))
            (Int.repr 56))) =
    Vlong (Int64.repr
      (ssz_u64_le_decode
        [byte0; byte1; byte2; byte3; byte4; byte5; byte6; byte7])).
Proof.
  intros byte0 byte1 byte2 byte3 byte4 byte5 byte6 byte7.
  f_equal.
  pose proof (Byte.unsigned_range_2 byte0) as Hbyte0.
  pose proof (Byte.unsigned_range_2 byte1) as Hbyte1.
  pose proof (Byte.unsigned_range_2 byte2) as Hbyte2.
  pose proof (Byte.unsigned_range_2 byte3) as Hbyte3.
  pose proof (Byte.unsigned_range_2 byte4) as Hbyte4.
  pose proof (Byte.unsigned_range_2 byte5) as Hbyte5.
  pose proof (Byte.unsigned_range_2 byte6) as Hbyte6.
  pose proof (Byte.unsigned_range_2 byte7) as Hbyte7.
  change Byte.max_unsigned with 255 in *.
  unfold ssz_u64_le_decode, byte_modulus.
  change (2 ^ 8) with 256.
  change (256 ^ 2) with 65536.
  change (256 ^ 3) with 16777216.
  change (256 ^ 4) with 4294967296.
  change (256 ^ 5) with 1099511627776.
  change (256 ^ 6) with 281474976710656.
  change (256 ^ 7) with 72057594037927936.
  replace (Int64.or (Int64.repr (Byte.unsigned byte0))
             (Int64.shl' (Int64.repr (Byte.unsigned byte1))
                (Int.repr 8)))
    with (Int64.repr
            (Byte.unsigned byte0 + 256 * Byte.unsigned byte1)).
  2: {
    symmetry; apply int64_or_shifted_byte; try reflexivity;
      change Int64.zwordsize with 64;
      change Int64.max_unsigned with 18446744073709551615; lia.
  }
  replace (Int64.or
             (Int64.repr
                (Byte.unsigned byte0 + 256 * Byte.unsigned byte1))
             (Int64.shl' (Int64.repr (Byte.unsigned byte2))
                (Int.repr 16)))
    with (Int64.repr
            (Byte.unsigned byte0 + 256 * Byte.unsigned byte1 +
             65536 * Byte.unsigned byte2)).
  2: {
    symmetry; apply int64_or_shifted_byte; try reflexivity;
      change Int64.zwordsize with 64;
      change Int64.max_unsigned with 18446744073709551615; lia.
  }
  replace (Int64.or
             (Int64.repr
                (Byte.unsigned byte0 + 256 * Byte.unsigned byte1 +
                 65536 * Byte.unsigned byte2))
             (Int64.shl' (Int64.repr (Byte.unsigned byte3))
                (Int.repr 24)))
    with (Int64.repr
            (Byte.unsigned byte0 + 256 * Byte.unsigned byte1 +
             65536 * Byte.unsigned byte2 +
             16777216 * Byte.unsigned byte3)).
  2: {
    symmetry; apply int64_or_shifted_byte; try reflexivity;
      change Int64.zwordsize with 64;
      change Int64.max_unsigned with 18446744073709551615; lia.
  }
  replace (Int64.or
             (Int64.repr
                (Byte.unsigned byte0 + 256 * Byte.unsigned byte1 +
                 65536 * Byte.unsigned byte2 +
                 16777216 * Byte.unsigned byte3))
             (Int64.shl' (Int64.repr (Byte.unsigned byte4))
                (Int.repr 32)))
    with (Int64.repr
            (Byte.unsigned byte0 + 256 * Byte.unsigned byte1 +
             65536 * Byte.unsigned byte2 +
             16777216 * Byte.unsigned byte3 +
             4294967296 * Byte.unsigned byte4)).
  2: {
    symmetry; apply int64_or_shifted_byte; try reflexivity;
      change Int64.zwordsize with 64;
      change Int64.max_unsigned with 18446744073709551615; lia.
  }
  replace (Int64.or
             (Int64.repr
                (Byte.unsigned byte0 + 256 * Byte.unsigned byte1 +
                 65536 * Byte.unsigned byte2 +
                 16777216 * Byte.unsigned byte3 +
                 4294967296 * Byte.unsigned byte4))
             (Int64.shl' (Int64.repr (Byte.unsigned byte5))
                (Int.repr 40)))
    with (Int64.repr
            (Byte.unsigned byte0 + 256 * Byte.unsigned byte1 +
             65536 * Byte.unsigned byte2 +
             16777216 * Byte.unsigned byte3 +
             4294967296 * Byte.unsigned byte4 +
             1099511627776 * Byte.unsigned byte5)).
  2: {
    symmetry; apply int64_or_shifted_byte; try reflexivity;
      change Int64.zwordsize with 64;
      change Int64.max_unsigned with 18446744073709551615; lia.
  }
  replace (Int64.or
             (Int64.repr
                (Byte.unsigned byte0 + 256 * Byte.unsigned byte1 +
                 65536 * Byte.unsigned byte2 +
                 16777216 * Byte.unsigned byte3 +
                 4294967296 * Byte.unsigned byte4 +
                 1099511627776 * Byte.unsigned byte5))
             (Int64.shl' (Int64.repr (Byte.unsigned byte6))
                (Int.repr 48)))
    with (Int64.repr
            (Byte.unsigned byte0 + 256 * Byte.unsigned byte1 +
             65536 * Byte.unsigned byte2 +
             16777216 * Byte.unsigned byte3 +
             4294967296 * Byte.unsigned byte4 +
             1099511627776 * Byte.unsigned byte5 +
             281474976710656 * Byte.unsigned byte6)).
  2: {
    symmetry; apply int64_or_shifted_byte; try reflexivity;
      change Int64.zwordsize with 64;
      change Int64.max_unsigned with 18446744073709551615; lia.
  }
  replace (Int64.or
             (Int64.repr
                (Byte.unsigned byte0 + 256 * Byte.unsigned byte1 +
                 65536 * Byte.unsigned byte2 +
                 16777216 * Byte.unsigned byte3 +
                 4294967296 * Byte.unsigned byte4 +
                 1099511627776 * Byte.unsigned byte5 +
                 281474976710656 * Byte.unsigned byte6))
             (Int64.shl' (Int64.repr (Byte.unsigned byte7))
                (Int.repr 56)))
    with (Int64.repr
            (Byte.unsigned byte0 + 256 * Byte.unsigned byte1 +
             65536 * Byte.unsigned byte2 +
             16777216 * Byte.unsigned byte3 +
             4294967296 * Byte.unsigned byte4 +
             1099511627776 * Byte.unsigned byte5 +
             281474976710656 * Byte.unsigned byte6 +
             72057594037927936 * Byte.unsigned byte7)).
  2: {
    symmetry; apply int64_or_shifted_byte; try reflexivity;
      change Int64.zwordsize with 64;
      change Int64.max_unsigned with 18446744073709551615; lia.
  }
	  f_equal; lia.
Qed.

Lemma tuchar_add_zero_field_address :
  forall p,
    field_compatible tuchar nil p ->
    force_val (sem_add_ptr_int tuchar Signed p (Vint (Int.repr 0))) =
    field_address tuchar nil p.
Proof.
  intros p Hfield.
  rewrite sem_add_pi' by
    (try reflexivity; try (eapply field_compatible_isptr; exact Hfield);
     change Int.min_signed with (-2147483648);
     change Int.max_signed with 2147483647; lia).
  change (sizeof tuchar) with 1.
  replace (1 * 0) with 0 by lia.
  rewrite (field_compatible_field_address _ _ _ Hfield).
  simpl.
  reflexivity.
Qed.

Lemma tuchar_add_offset_field_address :
  forall p ofs,
    field_compatible tuchar nil (offset_val ofs p) ->
    isptr p ->
    Int.min_signed <= ofs <= Int.max_signed ->
    force_val (sem_add_ptr_int tuchar Signed p (Vint (Int.repr ofs))) =
    field_address tuchar nil (offset_val ofs p).
Proof.
  intros p ofs Hfield Hisptr Hofs.
  rewrite sem_add_pi' by (try reflexivity; try exact Hisptr; lia).
  change (sizeof tuchar) with 1.
  replace (1 * ofs) with ofs by lia.
  rewrite (field_compatible_field_address _ _ _ Hfield).
  simpl.
  rewrite offset_offset_val.
  rewrite Z.add_0_r.
  reflexivity.
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

Lemma body_ssz_internal_write_u32_le :
  semax_body Vprog Gprog
    f_ssz_internal_write_u32_le ssz_internal_write_u32_le_spec.
Proof.
  start_function.
  unfold t_ssz_u32_bytes.
  rewrite data_at__tarray.
  change (Zrepeat (default_val tuchar) 4)
    with ([Vundef; Vundef; Vundef; Vundef]).
  forward.
  forward.
  forward.
  forward.
  unfold t_ssz_u32_bytes, ssz_u32_le_bytes,
    ssz_u32_le_byte0, ssz_u32_le_byte1,
    ssz_u32_le_byte2, ssz_u32_le_byte3.
  rewrite !Int.zero_ext_idem by lia.
  change
    (upd_Znth 3
       (upd_Znth 2
          (upd_Znth 1
             (upd_Znth 0 [Vundef; Vundef; Vundef; Vundef]
                (Vint
                   (Int.zero_ext 8
                      (Int.and (Int.repr value) (Int.repr 255)))))
             (Vint
                (Int.zero_ext 8
                   (Int.and (Int.shru (Int.repr value) (Int.repr 8))
                      (Int.repr 255)))))
          (Vint
             (Int.zero_ext 8
                (Int.and (Int.shru (Int.repr value) (Int.repr 16))
                   (Int.repr 255)))))
       (Vint
          (Int.zero_ext 8
             (Int.and (Int.shru (Int.repr value) (Int.repr 24))
                (Int.repr 255))))) with
    ([Vint (Int.zero_ext 8
              (Int.and (Int.repr value) (Int.repr 255)));
      Vint (Int.zero_ext 8
              (Int.and (Int.shru (Int.repr value) (Int.repr 8))
                 (Int.repr 255)));
      Vint (Int.zero_ext 8
              (Int.and (Int.shru (Int.repr value) (Int.repr 16))
                 (Int.repr 255)));
      Vint (Int.zero_ext 8
              (Int.and (Int.shru (Int.repr value) (Int.repr 24))
                 (Int.repr 255)))]).
  rewrite (write_u32_low_byte_value value H).
  rewrite (write_u32_byte1_value value H).
  rewrite (write_u32_byte2_value value H).
  rewrite (write_u32_byte3_value value H).
  entailer!.
Qed.

Lemma body_ssz_internal_read_u32_le :
  semax_body Vprog Gprog
    f_ssz_internal_read_u32_le ssz_internal_read_u32_le_spec.
Proof.
  start_function.
  forward.
  - change (Znth 0 (map Vubyte [byte0; byte1; byte2; byte3]))
      with (Vubyte byte0).
    unfold Vubyte.
    entailer!.
    pose proof (Byte.unsigned_range_2 byte0).
    lia.
  - forward.
    + change (Znth 1 (map Vubyte [byte0; byte1; byte2; byte3]))
        with (Vubyte byte1).
      unfold Vubyte.
      entailer!.
      pose proof (Byte.unsigned_range_2 byte1).
      lia.
    + forward.
      * change (Znth 2 (map Vubyte [byte0; byte1; byte2; byte3]))
          with (Vubyte byte2).
        unfold Vubyte.
        entailer!.
        pose proof (Byte.unsigned_range_2 byte2).
        lia.
      * forward.
        -- change (Znth 3 (map Vubyte [byte0; byte1; byte2; byte3]))
             with (Vubyte byte3).
           unfold Vubyte.
           entailer!.
           pose proof (Byte.unsigned_range_2 byte3).
           lia.
        -- forward.
           entailer!.
           change (Znth 0 [byte0; byte1; byte2; byte3]) with byte0.
           change (Znth 1 [byte0; byte1; byte2; byte3]) with byte1.
           change (Znth 2 [byte0; byte1; byte2; byte3]) with byte2.
           change (Znth 3 [byte0; byte1; byte2; byte3]) with byte3.
           unfold Vubyte.
           simpl.
           rewrite <- (zero_ext_u8_repr byte0).
           rewrite <- (zero_ext_u8_repr byte1).
           rewrite <- (zero_ext_u8_repr byte2).
           rewrite <- (zero_ext_u8_repr byte3).
	           apply read_u32_return_value.
Qed.

Local Opaque Int64.and Int64.loword Int64.shru'.

Lemma body_ssz_internal_write_u64_le :
  semax_body Vprog Gprog
    f_ssz_internal_write_u64_le ssz_internal_write_u64_le_spec.
Proof.
  start_function.
  unfold ssz_u64_le_data_at_.
  Intros.
  assert_PROP (field_compatible tuchar nil out) as FC0 by entailer!.
  pose proof (field_compatible_isptr _ _ _ FC0) as Hout_ptr.
  assert (Hout0 :
    force_val (sem_add_ptr_int tuchar Signed out (Vint (Int.repr 0))) =
    field_address tuchar nil out).
  { apply tuchar_add_zero_field_address; exact FC0. }
  forward.
  rewrite (write_u64_low_byte_value_store value H).
  assert_PROP (field_compatible tuchar nil (offset_val 1 out)) as FC1
    by entailer!.
  assert (Hout1 :
    force_val (sem_add_ptr_int tuchar Signed out (Vint (Int.repr 1))) =
    field_address tuchar nil (offset_val 1 out)).
  { apply tuchar_add_offset_field_address; try exact FC1; try exact Hout_ptr;
      change Int.min_signed with (-2147483648);
      change Int.max_signed with 2147483647; lia. }
  forward.
  rewrite (write_u64_byte1_value_store value H).
  assert_PROP (field_compatible tuchar nil (offset_val 2 out)) as FC2
    by entailer!.
  assert (Hout2 :
    force_val (sem_add_ptr_int tuchar Signed out (Vint (Int.repr 2))) =
    field_address tuchar nil (offset_val 2 out)).
  { apply tuchar_add_offset_field_address; try exact FC2; try exact Hout_ptr;
      change Int.min_signed with (-2147483648);
      change Int.max_signed with 2147483647; lia. }
  forward.
  rewrite (write_u64_byte2_value_store value H).
  assert_PROP (field_compatible tuchar nil (offset_val 3 out)) as FC3
    by entailer!.
  assert (Hout3 :
    force_val (sem_add_ptr_int tuchar Signed out (Vint (Int.repr 3))) =
    field_address tuchar nil (offset_val 3 out)).
  { apply tuchar_add_offset_field_address; try exact FC3; try exact Hout_ptr;
      change Int.min_signed with (-2147483648);
      change Int.max_signed with 2147483647; lia. }
  forward.
  rewrite (write_u64_byte3_value_store value H).
  assert_PROP (field_compatible tuchar nil (offset_val 4 out)) as FC4
    by entailer!.
  assert (Hout4 :
    force_val (sem_add_ptr_int tuchar Signed out (Vint (Int.repr 4))) =
    field_address tuchar nil (offset_val 4 out)).
  { apply tuchar_add_offset_field_address; try exact FC4; try exact Hout_ptr;
      change Int.min_signed with (-2147483648);
      change Int.max_signed with 2147483647; lia. }
  forward.
  rewrite (write_u64_byte4_value_store value H).
  assert_PROP (field_compatible tuchar nil (offset_val 5 out)) as FC5
    by entailer!.
  assert (Hout5 :
    force_val (sem_add_ptr_int tuchar Signed out (Vint (Int.repr 5))) =
    field_address tuchar nil (offset_val 5 out)).
  { apply tuchar_add_offset_field_address; try exact FC5; try exact Hout_ptr;
      change Int.min_signed with (-2147483648);
      change Int.max_signed with 2147483647; lia. }
  forward.
  rewrite (write_u64_byte5_value_store value H).
  assert_PROP (field_compatible tuchar nil (offset_val 6 out)) as FC6
    by entailer!.
  assert (Hout6 :
    force_val (sem_add_ptr_int tuchar Signed out (Vint (Int.repr 6))) =
    field_address tuchar nil (offset_val 6 out)).
  { apply tuchar_add_offset_field_address; try exact FC6; try exact Hout_ptr;
      change Int.min_signed with (-2147483648);
      change Int.max_signed with 2147483647; lia. }
  forward.
  rewrite (write_u64_byte6_value_store value H).
  assert_PROP (field_compatible tuchar nil (offset_val 7 out)) as FC7
    by entailer!.
  assert (Hout7 :
    force_val (sem_add_ptr_int tuchar Signed out (Vint (Int.repr 7))) =
    field_address tuchar nil (offset_val 7 out)).
  { apply tuchar_add_offset_field_address; try exact FC7; try exact Hout_ptr;
      change Int.min_signed with (-2147483648);
      change Int.max_signed with 2147483647; lia. }
  forward.
  rewrite (write_u64_byte7_value_store value H).
  entailer!.
Qed.

Lemma body_ssz_internal_read_u64_le :
  semax_body Vprog Gprog
    f_ssz_internal_read_u64_le ssz_internal_read_u64_le_spec.
Proof.
  start_function.
  assert_PROP (field_compatible tuchar nil input) as FC0 by entailer!.
  pose proof (field_compatible_isptr _ _ _ FC0) as Hinput_ptr.
  assert (Hinput0 :
    force_val (sem_add_ptr_int tuchar Signed input (Vint (Int.repr 0))) =
    field_address tuchar nil input).
  { apply tuchar_add_zero_field_address; exact FC0. }
  forward; [
    unfold Vubyte; entailer!;
    pose proof (Byte.unsigned_range_2 byte0); lia |].
  assert_PROP (field_compatible tuchar nil (offset_val 1 input)) as FC1
    by entailer!.
  assert (Hinput1 :
    force_val (sem_add_ptr_int tuchar Signed input (Vint (Int.repr 1))) =
    field_address tuchar nil (offset_val 1 input)).
  { apply tuchar_add_offset_field_address; try exact FC1; try exact Hinput_ptr;
      change Int.min_signed with (-2147483648);
      change Int.max_signed with 2147483647; lia. }
  forward; [
    unfold Vubyte; entailer!;
    pose proof (Byte.unsigned_range_2 byte1); lia |].
  assert_PROP (field_compatible tuchar nil (offset_val 2 input)) as FC2
    by entailer!.
  assert (Hinput2 :
    force_val (sem_add_ptr_int tuchar Signed input (Vint (Int.repr 2))) =
    field_address tuchar nil (offset_val 2 input)).
  { apply tuchar_add_offset_field_address; try exact FC2; try exact Hinput_ptr;
      change Int.min_signed with (-2147483648);
      change Int.max_signed with 2147483647; lia. }
  forward; [
    unfold Vubyte; entailer!;
    pose proof (Byte.unsigned_range_2 byte2); lia |].
  assert_PROP (field_compatible tuchar nil (offset_val 3 input)) as FC3
    by entailer!.
  assert (Hinput3 :
    force_val (sem_add_ptr_int tuchar Signed input (Vint (Int.repr 3))) =
    field_address tuchar nil (offset_val 3 input)).
  { apply tuchar_add_offset_field_address; try exact FC3; try exact Hinput_ptr;
      change Int.min_signed with (-2147483648);
      change Int.max_signed with 2147483647; lia. }
  forward; [
    unfold Vubyte; entailer!;
    pose proof (Byte.unsigned_range_2 byte3); lia |].
  assert_PROP (field_compatible tuchar nil (offset_val 4 input)) as FC4
    by entailer!.
  assert (Hinput4 :
    force_val (sem_add_ptr_int tuchar Signed input (Vint (Int.repr 4))) =
    field_address tuchar nil (offset_val 4 input)).
  { apply tuchar_add_offset_field_address; try exact FC4; try exact Hinput_ptr;
      change Int.min_signed with (-2147483648);
      change Int.max_signed with 2147483647; lia. }
  forward; [
    unfold Vubyte; entailer!;
    pose proof (Byte.unsigned_range_2 byte4); lia |].
  assert_PROP (field_compatible tuchar nil (offset_val 5 input)) as FC5
    by entailer!.
  assert (Hinput5 :
    force_val (sem_add_ptr_int tuchar Signed input (Vint (Int.repr 5))) =
    field_address tuchar nil (offset_val 5 input)).
  { apply tuchar_add_offset_field_address; try exact FC5; try exact Hinput_ptr;
      change Int.min_signed with (-2147483648);
      change Int.max_signed with 2147483647; lia. }
  forward; [
    unfold Vubyte; entailer!;
    pose proof (Byte.unsigned_range_2 byte5); lia |].
  assert_PROP (field_compatible tuchar nil (offset_val 6 input)) as FC6
    by entailer!.
  assert (Hinput6 :
    force_val (sem_add_ptr_int tuchar Signed input (Vint (Int.repr 6))) =
    field_address tuchar nil (offset_val 6 input)).
  { apply tuchar_add_offset_field_address; try exact FC6; try exact Hinput_ptr;
      change Int.min_signed with (-2147483648);
      change Int.max_signed with 2147483647; lia. }
  forward; [
    unfold Vubyte; entailer!;
    pose proof (Byte.unsigned_range_2 byte6); lia |].
  assert_PROP (field_compatible tuchar nil (offset_val 7 input)) as FC7
    by entailer!.
  assert (Hinput7 :
    force_val (sem_add_ptr_int tuchar Signed input (Vint (Int.repr 7))) =
    field_address tuchar nil (offset_val 7 input)).
  { apply tuchar_add_offset_field_address; try exact FC7; try exact Hinput_ptr;
      change Int.min_signed with (-2147483648);
      change Int.max_signed with 2147483647; lia. }
  forward; [
    unfold Vubyte; entailer!;
    pose proof (Byte.unsigned_range_2 byte7); lia |].
  forward.
  entailer!.
  change (Znth 0 [byte0; byte1; byte2; byte3; byte4; byte5; byte6; byte7])
    with byte0.
  change (Znth 1 [byte0; byte1; byte2; byte3; byte4; byte5; byte6; byte7])
    with byte1.
  change (Znth 2 [byte0; byte1; byte2; byte3; byte4; byte5; byte6; byte7])
    with byte2.
  change (Znth 3 [byte0; byte1; byte2; byte3; byte4; byte5; byte6; byte7])
    with byte3.
  change (Znth 4 [byte0; byte1; byte2; byte3; byte4; byte5; byte6; byte7])
    with byte4.
  change (Znth 5 [byte0; byte1; byte2; byte3; byte4; byte5; byte6; byte7])
    with byte5.
  change (Znth 6 [byte0; byte1; byte2; byte3; byte4; byte5; byte6; byte7])
    with byte6.
  change (Znth 7 [byte0; byte1; byte2; byte3; byte4; byte5; byte6; byte7])
    with byte7.
  unfold Vubyte.
  simpl.
  try rewrite (zero_ext_u8_repr byte0).
  try rewrite (zero_ext_u8_repr byte1).
  try rewrite (zero_ext_u8_repr byte2).
  try rewrite (zero_ext_u8_repr byte3).
  try rewrite (zero_ext_u8_repr byte4).
  try rewrite (zero_ext_u8_repr byte5).
  try rewrite (zero_ext_u8_repr byte6).
  try rewrite (zero_ext_u8_repr byte7).
  pose proof (Byte.unsigned_range_2 byte0) as Hbyte0.
  pose proof (Byte.unsigned_range_2 byte1) as Hbyte1.
  pose proof (Byte.unsigned_range_2 byte2) as Hbyte2.
  pose proof (Byte.unsigned_range_2 byte3) as Hbyte3.
  pose proof (Byte.unsigned_range_2 byte4) as Hbyte4.
  pose proof (Byte.unsigned_range_2 byte5) as Hbyte5.
  pose proof (Byte.unsigned_range_2 byte6) as Hbyte6.
  pose proof (Byte.unsigned_range_2 byte7) as Hbyte7.
  change Byte.max_unsigned with 255 in *.
  rewrite !Int.unsigned_repr by
    (change Int.max_unsigned with 4294967295; lia).
  change (Int64.shl (Int64.repr (Byte.unsigned byte1)) (Int64.repr 8))
    with (Int64.shl' (Int64.repr (Byte.unsigned byte1)) (Int.repr 8)).
  change (Int64.shl (Int64.repr (Byte.unsigned byte2)) (Int64.repr 16))
    with (Int64.shl' (Int64.repr (Byte.unsigned byte2)) (Int.repr 16)).
  change (Int64.shl (Int64.repr (Byte.unsigned byte3)) (Int64.repr 24))
    with (Int64.shl' (Int64.repr (Byte.unsigned byte3)) (Int.repr 24)).
  change (Int64.shl (Int64.repr (Byte.unsigned byte4)) (Int64.repr 32))
    with (Int64.shl' (Int64.repr (Byte.unsigned byte4)) (Int.repr 32)).
  change (Int64.shl (Int64.repr (Byte.unsigned byte5)) (Int64.repr 40))
    with (Int64.shl' (Int64.repr (Byte.unsigned byte5)) (Int.repr 40)).
  change (Int64.shl (Int64.repr (Byte.unsigned byte6)) (Int64.repr 48))
    with (Int64.shl' (Int64.repr (Byte.unsigned byte6)) (Int.repr 48)).
  change (Int64.shl (Int64.repr (Byte.unsigned byte7)) (Int64.repr 56))
    with (Int64.shl' (Int64.repr (Byte.unsigned byte7)) (Int.repr 56)).
  change (Byte.unsigned byte0 + byte_modulus * Byte.unsigned byte1 +
    byte_modulus ^ 2 * Byte.unsigned byte2 +
    byte_modulus ^ 3 * Byte.unsigned byte3 +
    byte_modulus ^ 4 * Byte.unsigned byte4 +
    byte_modulus ^ 5 * Byte.unsigned byte5 +
    byte_modulus ^ 6 * Byte.unsigned byte6 +
    byte_modulus ^ 7 * Byte.unsigned byte7)
    with (ssz_u64_le_decode
      [byte0; byte1; byte2; byte3; byte4; byte5; byte6; byte7]).
  exact (read_u64_return_value byte0 byte1 byte2 byte3 byte4 byte5 byte6 byte7).
Qed.
