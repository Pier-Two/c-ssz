From Stdlib Require Import ZArith List Lia.
From compcert Require Import Integers.

Import ListNotations.
Local Open Scope Z_scope.

Definition byte_modulus : Z := 2 ^ 8.
Definition uint16_modulus : Z := 2 ^ 16.
Definition uint16_max : Z := uint16_modulus - 1.
Definition uint32_modulus : Z := 2 ^ 32.
Definition uint32_max : Z := uint32_modulus - 1.
Definition uint64_modulus : Z := 2 ^ 64.
Definition uint64_max : Z := uint64_modulus - 1.

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

Definition ssz_u32_le_byte0 (value : Z) : byte :=
  Byte.repr (value mod byte_modulus).

Definition ssz_u32_le_byte1 (value : Z) : byte :=
  Byte.repr ((value / byte_modulus) mod byte_modulus).

Definition ssz_u32_le_byte2 (value : Z) : byte :=
  Byte.repr ((value / byte_modulus ^ 2) mod byte_modulus).

Definition ssz_u32_le_byte3 (value : Z) : byte :=
  Byte.repr ((value / byte_modulus ^ 3) mod byte_modulus).

Definition ssz_u32_le_bytes (value : Z) : list byte :=
  [ssz_u32_le_byte0 value; ssz_u32_le_byte1 value;
   ssz_u32_le_byte2 value; ssz_u32_le_byte3 value].

Definition ssz_u32_le_decode (bytes : list byte) : Z :=
  match bytes with
  | [byte0; byte1; byte2; byte3] =>
      Byte.unsigned byte0 +
      byte_modulus * Byte.unsigned byte1 +
      byte_modulus ^ 2 * Byte.unsigned byte2 +
      byte_modulus ^ 3 * Byte.unsigned byte3
  | _ => 0
  end.

Definition ssz_u64_le_byte0 (value : Z) : byte :=
  Byte.repr (value mod byte_modulus).

Definition ssz_u64_le_byte1 (value : Z) : byte :=
  Byte.repr ((value / byte_modulus) mod byte_modulus).

Definition ssz_u64_le_byte2 (value : Z) : byte :=
  Byte.repr ((value / byte_modulus ^ 2) mod byte_modulus).

Definition ssz_u64_le_byte3 (value : Z) : byte :=
  Byte.repr ((value / byte_modulus ^ 3) mod byte_modulus).

Definition ssz_u64_le_byte4 (value : Z) : byte :=
  Byte.repr ((value / byte_modulus ^ 4) mod byte_modulus).

Definition ssz_u64_le_byte5 (value : Z) : byte :=
  Byte.repr ((value / byte_modulus ^ 5) mod byte_modulus).

Definition ssz_u64_le_byte6 (value : Z) : byte :=
  Byte.repr ((value / byte_modulus ^ 6) mod byte_modulus).

Definition ssz_u64_le_byte7 (value : Z) : byte :=
  Byte.repr ((value / byte_modulus ^ 7) mod byte_modulus).

Definition ssz_u64_le_bytes (value : Z) : list byte :=
  [ssz_u64_le_byte0 value; ssz_u64_le_byte1 value;
   ssz_u64_le_byte2 value; ssz_u64_le_byte3 value;
   ssz_u64_le_byte4 value; ssz_u64_le_byte5 value;
   ssz_u64_le_byte6 value; ssz_u64_le_byte7 value].

Definition ssz_u64_le_decode (bytes : list byte) : Z :=
  match bytes with
  | [byte0; byte1; byte2; byte3; byte4; byte5; byte6; byte7] =>
      Byte.unsigned byte0 +
      byte_modulus * Byte.unsigned byte1 +
      byte_modulus ^ 2 * Byte.unsigned byte2 +
      byte_modulus ^ 3 * Byte.unsigned byte3 +
      byte_modulus ^ 4 * Byte.unsigned byte4 +
      byte_modulus ^ 5 * Byte.unsigned byte5 +
      byte_modulus ^ 6 * Byte.unsigned byte6 +
      byte_modulus ^ 7 * Byte.unsigned byte7
  | _ => 0
  end.

Lemma ssz_u16_le_bytes_length :
  forall value, Zlength (ssz_u16_le_bytes value) = 2.
Proof.
  intros; reflexivity.
Qed.

Lemma ssz_u32_le_bytes_length :
  forall value, Zlength (ssz_u32_le_bytes value) = 4.
Proof.
  intros; reflexivity.
Qed.

Lemma ssz_u64_le_bytes_length :
  forall value, Zlength (ssz_u64_le_bytes value) = 8.
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

Lemma ssz_u32_le_decode_range :
  forall byte0 byte1 byte2 byte3,
    0 <= ssz_u32_le_decode [byte0; byte1; byte2; byte3] <= uint32_max.
Proof.
  intros byte0 byte1 byte2 byte3.
  unfold ssz_u32_le_decode, byte_modulus, uint32_max, uint32_modulus.
  change (2 ^ 8) with 256.
  change (2 ^ 32) with 4294967296.
  pose proof (Byte.unsigned_range_2 byte0).
  pose proof (Byte.unsigned_range_2 byte1).
  pose proof (Byte.unsigned_range_2 byte2).
  pose proof (Byte.unsigned_range_2 byte3).
  change Byte.max_unsigned with 255 in *.
  lia.
Qed.

Lemma ssz_u64_le_decode_range :
  forall byte0 byte1 byte2 byte3 byte4 byte5 byte6 byte7,
    0 <= ssz_u64_le_decode
      [byte0; byte1; byte2; byte3; byte4; byte5; byte6; byte7] <=
    uint64_max.
Proof.
  intros byte0 byte1 byte2 byte3 byte4 byte5 byte6 byte7.
  unfold ssz_u64_le_decode, byte_modulus, uint64_max, uint64_modulus.
  change (2 ^ 8) with 256.
  change (2 ^ 64) with 18446744073709551616.
  pose proof (Byte.unsigned_range_2 byte0).
  pose proof (Byte.unsigned_range_2 byte1).
  pose proof (Byte.unsigned_range_2 byte2).
  pose proof (Byte.unsigned_range_2 byte3).
  pose proof (Byte.unsigned_range_2 byte4).
  pose proof (Byte.unsigned_range_2 byte5).
  pose proof (Byte.unsigned_range_2 byte6).
  pose proof (Byte.unsigned_range_2 byte7).
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

Lemma ssz_u32_le_decode_encode_roundtrip :
  forall value,
    0 <= value <= uint32_max ->
    ssz_u32_le_decode (ssz_u32_le_bytes value) = value.
Proof.
  intros value Hrange.
  unfold ssz_u32_le_decode, ssz_u32_le_bytes,
    ssz_u32_le_byte0, ssz_u32_le_byte1,
    ssz_u32_le_byte2, ssz_u32_le_byte3.
  assert (Hbyte0 : 0 <= value mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound value 256 ltac:(lia)).
    lia.
  }
  assert (Hbyte1 :
    0 <= (value / byte_modulus) mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound (value / 256) 256 ltac:(lia)).
    lia.
  }
  assert (Hbyte2 :
    0 <= (value / byte_modulus ^ 2) mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 2) with 65536.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound (value / 65536) 256 ltac:(lia)).
    lia.
  }
  assert (Hbyte3 :
    0 <= (value / byte_modulus ^ 3) mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 3) with 16777216.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound (value / 16777216) 256 ltac:(lia)).
    lia.
  }
  rewrite Byte.unsigned_repr by exact Hbyte0.
  rewrite Byte.unsigned_repr by exact Hbyte1.
  rewrite Byte.unsigned_repr by exact Hbyte2.
  rewrite Byte.unsigned_repr by exact Hbyte3.
  unfold byte_modulus.
  change (2 ^ 8) with 256.
  change (256 ^ 2) with 65536.
  change (256 ^ 3) with 16777216.
  set (q1 := value / 256).
  set (q2 := value / 65536).
  set (q3 := value / 16777216).
  set (r0 := value mod 256).
  set (r1 := value / 256 mod 256).
  set (r2 := value / 65536 mod 256).
  set (r3 := value / 16777216 mod 256).
  change (r0 + 256 * r1 + 65536 * r2 + 16777216 * r3 = value).
  assert (Hvalue_lt : value < 4294967296).
  {
    unfold uint32_max, uint32_modulus in Hrange.
    change (2 ^ 32) with 4294967296 in Hrange.
    lia.
  }
  assert (Hstep0 : value = q1 * 256 + r0).
  {
    subst q1 r0.
    pose proof (Z.div_mod value 256 ltac:(lia)) as Hstep.
    rewrite Z.mul_comm in Hstep.
    exact Hstep.
  }
  assert (Hstep1 : q1 = q2 * 256 + r1).
  {
    subst q1 q2 r1.
    pose proof (Z.div_mod (value / 256) 256 ltac:(lia)) as Hstep.
    replace (value / 256 / 256) with (value / 65536) in Hstep by
      (rewrite Z.div_div by lia; reflexivity).
    lia.
  }
  assert (Hstep2 : q2 = q3 * 256 + r2).
  {
    subst q2 q3 r2.
    pose proof (Z.div_mod (value / 65536) 256 ltac:(lia)) as Hstep.
    replace (value / 65536 / 256) with (value / 16777216) in Hstep by
      (rewrite Z.div_div by lia; reflexivity).
    lia.
  }
  assert (Hq3 : 0 <= value / 16777216 < 256).
  {
    split.
    - apply Z.div_pos; lia.
    - apply Z.div_lt_upper_bound; lia.
  }
  assert (Hstep3 : r3 = q3).
  {
    subst q3 r3.
    rewrite Z.mod_small; lia.
  }
  nia.
Qed.

Lemma ssz_u32_le_bytes_injective :
  forall value1 value2,
    0 <= value1 <= uint32_max ->
    0 <= value2 <= uint32_max ->
    value1 <> value2 ->
    ssz_u32_le_bytes value1 <> ssz_u32_le_bytes value2.
Proof.
  intros value1 value2 Hrange1 Hrange2 Hneq Hbytes.
  apply Hneq.
  pose proof (f_equal ssz_u32_le_decode Hbytes) as Hdecoded.
  rewrite !ssz_u32_le_decode_encode_roundtrip in Hdecoded by assumption.
  exact Hdecoded.
Qed.

Lemma ssz_u64_le_decode_encode_roundtrip :
  forall value,
    0 <= value <= uint64_max ->
    ssz_u64_le_decode (ssz_u64_le_bytes value) = value.
Proof.
  intros value Hrange.
  unfold ssz_u64_le_decode, ssz_u64_le_bytes,
    ssz_u64_le_byte0, ssz_u64_le_byte1,
    ssz_u64_le_byte2, ssz_u64_le_byte3,
    ssz_u64_le_byte4, ssz_u64_le_byte5,
    ssz_u64_le_byte6, ssz_u64_le_byte7.
  assert (Hbyte0 : 0 <= value mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound value 256 ltac:(lia)).
    lia.
  }
  assert (Hbyte1 :
    0 <= (value / byte_modulus) mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound (value / 256) 256 ltac:(lia)).
    lia.
  }
  assert (Hbyte2 :
    0 <= (value / byte_modulus ^ 2) mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 2) with 65536.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound (value / 65536) 256 ltac:(lia)).
    lia.
  }
  assert (Hbyte3 :
    0 <= (value / byte_modulus ^ 3) mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 3) with 16777216.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound (value / 16777216) 256 ltac:(lia)).
    lia.
  }
  assert (Hbyte4 :
    0 <= (value / byte_modulus ^ 4) mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 4) with 4294967296.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound (value / 4294967296) 256 ltac:(lia)).
    lia.
  }
  assert (Hbyte5 :
    0 <= (value / byte_modulus ^ 5) mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 5) with 1099511627776.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound (value / 1099511627776) 256 ltac:(lia)).
    lia.
  }
  assert (Hbyte6 :
    0 <= (value / byte_modulus ^ 6) mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 6) with 281474976710656.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound (value / 281474976710656) 256 ltac:(lia)).
    lia.
  }
  assert (Hbyte7 :
    0 <= (value / byte_modulus ^ 7) mod byte_modulus <= Byte.max_unsigned).
  {
    unfold byte_modulus.
    change (2 ^ 8) with 256.
    change (256 ^ 7) with 72057594037927936.
    change Byte.max_unsigned with 255.
    pose proof (Z.mod_pos_bound (value / 72057594037927936) 256 ltac:(lia)).
    lia.
  }
  rewrite Byte.unsigned_repr by exact Hbyte0.
  rewrite Byte.unsigned_repr by exact Hbyte1.
  rewrite Byte.unsigned_repr by exact Hbyte2.
  rewrite Byte.unsigned_repr by exact Hbyte3.
  rewrite Byte.unsigned_repr by exact Hbyte4.
  rewrite Byte.unsigned_repr by exact Hbyte5.
  rewrite Byte.unsigned_repr by exact Hbyte6.
  rewrite Byte.unsigned_repr by exact Hbyte7.
  unfold byte_modulus.
  change (2 ^ 8) with 256.
  change (256 ^ 2) with 65536.
  change (256 ^ 3) with 16777216.
  change (256 ^ 4) with 4294967296.
  change (256 ^ 5) with 1099511627776.
  change (256 ^ 6) with 281474976710656.
  change (256 ^ 7) with 72057594037927936.
  set (q1 := value / 256).
  set (q2 := value / 65536).
  set (q3 := value / 16777216).
  set (q4 := value / 4294967296).
  set (q5 := value / 1099511627776).
  set (q6 := value / 281474976710656).
  set (q7 := value / 72057594037927936).
  set (r0 := value mod 256).
  set (r1 := value / 256 mod 256).
  set (r2 := value / 65536 mod 256).
  set (r3 := value / 16777216 mod 256).
  set (r4 := value / 4294967296 mod 256).
  set (r5 := value / 1099511627776 mod 256).
  set (r6 := value / 281474976710656 mod 256).
  set (r7 := value / 72057594037927936 mod 256).
  change
    (r0 + 256 * r1 + 65536 * r2 + 16777216 * r3 +
     4294967296 * r4 + 1099511627776 * r5 +
     281474976710656 * r6 + 72057594037927936 * r7 = value).
  assert (Hvalue_lt : value < 18446744073709551616).
  {
    unfold uint64_max, uint64_modulus in Hrange.
    change (2 ^ 64) with 18446744073709551616 in Hrange.
    lia.
  }
  assert (Hstep0 : value = q1 * 256 + r0).
  {
    subst q1 r0.
    pose proof (Z.div_mod value 256 ltac:(lia)).
    lia.
  }
  assert (Hstep1 : q1 = q2 * 256 + r1).
  {
    subst q1 q2 r1.
    pose proof (Z.div_mod (value / 256) 256 ltac:(lia)) as Hstep.
    replace (value / 256 / 256) with (value / 65536) in Hstep by
      (rewrite Z.div_div by lia; reflexivity).
    rewrite Z.mul_comm in Hstep.
    exact Hstep.
  }
  assert (Hstep2 : q2 = q3 * 256 + r2).
  {
    subst q2 q3 r2.
    pose proof (Z.div_mod (value / 65536) 256 ltac:(lia)) as Hstep.
    replace (value / 65536 / 256) with (value / 16777216) in Hstep by
      (rewrite Z.div_div by lia; reflexivity).
    rewrite Z.mul_comm in Hstep.
    exact Hstep.
  }
  assert (Hstep3 : q3 = q4 * 256 + r3).
  {
    subst q3 q4 r3.
    pose proof (Z.div_mod (value / 16777216) 256 ltac:(lia)) as Hstep.
    replace (value / 16777216 / 256) with (value / 4294967296) in Hstep by
      (rewrite Z.div_div by lia; reflexivity).
    rewrite Z.mul_comm in Hstep.
    exact Hstep.
  }
  assert (Hstep4 : q4 = q5 * 256 + r4).
  {
    subst q4 q5 r4.
    pose proof (Z.div_mod (value / 4294967296) 256 ltac:(lia)) as Hstep.
    replace (value / 4294967296 / 256)
      with (value / 1099511627776) in Hstep by
      (rewrite Z.div_div by lia; reflexivity).
    rewrite Z.mul_comm in Hstep.
    exact Hstep.
  }
  assert (Hstep5 : q5 = q6 * 256 + r5).
  {
    subst q5 q6 r5.
    pose proof (Z.div_mod (value / 1099511627776) 256 ltac:(lia)) as Hstep.
    replace (value / 1099511627776 / 256)
      with (value / 281474976710656) in Hstep by
      (rewrite Z.div_div by lia; reflexivity).
    rewrite Z.mul_comm in Hstep.
    exact Hstep.
  }
  assert (Hstep6 : q6 = q7 * 256 + r6).
  {
    subst q6 q7 r6.
    pose proof (Z.div_mod (value / 281474976710656) 256 ltac:(lia)) as Hstep.
    replace (value / 281474976710656 / 256)
      with (value / 72057594037927936) in Hstep by
      (rewrite Z.div_div by lia; reflexivity).
    rewrite Z.mul_comm in Hstep.
    exact Hstep.
  }
  assert (Hq7 : 0 <= value / 72057594037927936 < 256).
  {
    split.
    - apply Z.div_pos; lia.
    - apply Z.div_lt_upper_bound; lia.
  }
  assert (Hstep7 : r7 = q7).
  {
    subst q7 r7.
    rewrite Z.mod_small; lia.
  }
  lia.
Qed.

Lemma ssz_u64_le_bytes_injective :
  forall value1 value2,
    0 <= value1 <= uint64_max ->
    0 <= value2 <= uint64_max ->
    value1 <> value2 ->
    ssz_u64_le_bytes value1 <> ssz_u64_le_bytes value2.
Proof.
  intros value1 value2 Hrange1 Hrange2 Hneq Hbytes.
  apply Hneq.
  pose proof (f_equal ssz_u64_le_decode Hbytes) as Hdecoded.
  rewrite !ssz_u64_le_decode_encode_roundtrip in Hdecoded by assumption.
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
