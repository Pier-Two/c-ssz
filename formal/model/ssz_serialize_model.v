From Stdlib Require Import ZArith List.
From compcert Require Import Integers.
Require Import VST.zlist.sublist.
Require Import SSZFormal.model.ssz_endian_model.

Import ListNotations.
Local Open Scope Z_scope.

Definition byte_inhabitant : Inhabitant byte := Byte.zero.

Definition ssz_success : Z := 0.
Definition ssz_err_invalid_argument : Z := 1.
Definition ssz_err_buffer_too_small : Z := 2.
Definition ssz_err_overflow : Z := 3.
Definition ssz_err_limit_exceeded : Z := 4.
Definition ssz_err_schema_invalid : Z := 5.
Definition ssz_err_encoding_invalid : Z := 6.

Definition ssz_internal_prepare_output_result
    (has_out_len has_out : bool) (required out_cap : Z) : Z :=
  if negb has_out_len then ssz_err_invalid_argument
  else if andb has_out (out_cap <? required)
       then ssz_err_buffer_too_small
       else ssz_success.

Definition ssz_internal_prepare_output_len (required : Z) : Z :=
  required.

Definition ssz_serialize_uint8_byte (value : Z) : byte :=
  Byte.repr value.

Definition ssz_serialize_uint8_bytes (value : Z) : list byte :=
  [ssz_serialize_uint8_byte value].

Definition ssz_serialize_boolean_byte (value : Z) : byte :=
  Byte.repr value.

Definition ssz_serialize_boolean_result (has_out : bool) (value : Z) : Z :=
  if negb has_out then ssz_err_invalid_argument
  else if value <=? 1 then ssz_success else ssz_err_encoding_invalid.

Definition ssz_serialize_bitvector_zero_result (bit_count : Z) : Z :=
  if bit_count =? 0 then ssz_err_schema_invalid else ssz_success.

Definition ssz_serialize_bitlist_limit_result
    (bit_len bit_limit : Z) : Z :=
  if andb (negb (bit_limit =? uint64_max)) (bit_limit <? bit_len)
  then ssz_err_limit_exceeded
  else ssz_success.

Definition ssz_bits_to_bytes (bit_count : Z) : Z :=
  (bit_count + 7) / 8.

Definition ssz_bits_to_bytes_ok (bit_count : Z) : bool :=
  ssz_bits_to_bytes bit_count <=? uint64_max.

Definition ssz_byte_mask (used_bits : Z) : Z :=
  (2 ^ used_bits) - 1.

Definition ssz_padding_valid (bit_count : Z) (bytes : list byte) : bool :=
  if bit_count mod 8 =? 0 then true
  else
    let byte_count := ssz_bits_to_bytes bit_count in
    let last := @Znth byte byte_inhabitant (byte_count - 1) bytes in
    let mask := ssz_byte_mask (bit_count mod 8) in
    Int.eq
      (Int.and (Int.repr (Byte.unsigned last))
        (Int.zero_ext 8 (Int.not (Int.zero_ext 8 (Int.repr mask)))))
      Int.zero.

Definition ssz_serialize_bitvector_result
    (has_bits has_out_len has_out : bool)
    (bits_le_len bit_count out_cap : Z) (bytes : list byte) : Z :=
  if bit_count =? 0 then ssz_err_schema_invalid
  else if negb (ssz_bits_to_bytes_ok bit_count) then ssz_err_overflow
  else
    let byte_count := ssz_bits_to_bytes bit_count in
    if orb (negb has_bits) (bits_le_len <? byte_count)
    then ssz_err_invalid_argument
    else if negb (ssz_padding_valid bit_count bytes)
         then ssz_err_encoding_invalid
         else ssz_internal_prepare_output_result
                has_out_len has_out byte_count out_cap.

Definition ssz_serialize_bitvector_bytes (bytes : list byte) : list byte :=
  bytes.

Definition ssz_bitlist_data_bytes (bit_len : Z) : Z :=
  ssz_bits_to_bytes bit_len.

Definition ssz_bitlist_delimiter_byte (bit_len : Z) : Z :=
  bit_len / 8.

Definition ssz_bitlist_required (bit_len : Z) : Z :=
  ssz_bitlist_delimiter_byte bit_len + 1.

Definition ssz_bitlist_delimiter_bit (bit_len : Z) : Z :=
  2 ^ (bit_len mod 8).

Definition ssz_or_delimiter_byte (b : byte) (bit_len : Z) : byte :=
  Byte.repr
    (Int.unsigned
      (Int.zero_ext 8
        (Int.or (Int.repr (Byte.unsigned b))
          (Int.repr (ssz_bitlist_delimiter_bit bit_len))))).

Definition ssz_serialize_bitlist_bytes
    (bit_len : Z) (bytes : list byte) : list byte :=
  if bit_len mod 8 =? 0
  then bytes ++ [Byte.repr (ssz_bitlist_delimiter_bit bit_len)]
  else
    let data_bytes := ssz_bitlist_data_bytes bit_len in
    let last := @Znth byte byte_inhabitant (data_bytes - 1) bytes in
    sublist 0 (data_bytes - 1) bytes ++
      [ssz_or_delimiter_byte last bit_len].

Definition ssz_serialize_bitlist_result
    (has_bits has_out_len has_out : bool)
    (bits_le_len bit_len bit_limit out_cap : Z) (bytes : list byte) : Z :=
  if andb (negb (bit_limit =? uint64_max)) (bit_limit <? bit_len)
  then ssz_err_limit_exceeded
  else if negb (ssz_bits_to_bytes_ok bit_len) then ssz_err_overflow
  else
    let data_bytes := ssz_bitlist_data_bytes bit_len in
    let required := ssz_bitlist_required bit_len in
    if andb (negb (data_bytes =? 0))
         (orb (negb has_bits) (bits_le_len <? data_bytes))
    then ssz_err_invalid_argument
    else if negb (ssz_padding_valid bit_len bytes)
         then ssz_err_encoding_invalid
         else ssz_internal_prepare_output_result
                has_out_len has_out required out_cap.

Definition ssz_serialize_uint16_bytes (value : Z) : list byte :=
  ssz_u16_le_bytes value.

Definition ssz_serialize_uint32_bytes (value : Z) : list byte :=
  ssz_u32_le_bytes value.

Definition ssz_serialize_uint64_bytes (value : Z) : list byte :=
  ssz_u64_le_bytes value.

Definition ssz_serialize_uint128_bytes (value : list byte) : list byte :=
  value.

Definition ssz_serialize_uint256_bytes (value : list byte) : list byte :=
  value.

Definition ssz_serialize_fixed_bytes_result
    (has_value has_out : bool) (expected_len value_len : Z) : Z :=
  if orb (negb has_value) (negb has_out) then ssz_err_invalid_argument
  else if value_len =? expected_len
       then ssz_success
       else ssz_err_encoding_invalid.

Lemma ssz_serialize_uint8_bytes_length :
  forall value, Zlength (ssz_serialize_uint8_bytes value) = 1.
Proof.
  intros; reflexivity.
Qed.

Lemma ssz_serialize_uint16_bytes_length :
  forall value, Zlength (ssz_serialize_uint16_bytes value) = 2.
Proof.
  intros; apply ssz_u16_le_bytes_length.
Qed.

Lemma ssz_serialize_uint32_bytes_length :
  forall value, Zlength (ssz_serialize_uint32_bytes value) = 4.
Proof.
  intros; apply ssz_u32_le_bytes_length.
Qed.

Lemma ssz_serialize_uint64_bytes_length :
  forall value, Zlength (ssz_serialize_uint64_bytes value) = 8.
Proof.
  intros; apply ssz_u64_le_bytes_length.
Qed.

Lemma ssz_serialize_uint128_bytes_length :
  forall value,
    Zlength value = 16 ->
    Zlength (ssz_serialize_uint128_bytes value) = 16.
Proof.
  intros value Hlen; exact Hlen.
Qed.

Lemma ssz_serialize_uint256_bytes_length :
  forall value,
    Zlength value = 32 ->
    Zlength (ssz_serialize_uint256_bytes value) = 32.
Proof.
  intros value Hlen; exact Hlen.
Qed.
