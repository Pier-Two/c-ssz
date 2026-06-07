From Stdlib Require Import ZArith List Lia.
From compcert Require Import Integers.
Require Import VST.floyd.proofauto.
Require Import SSZFormal.clight.ssz_serialize.
Require Import SSZFormal.model.ssz_endian_model.
Require Import SSZFormal.model.ssz_serialize_model.
Require Import SSZFormal.spec.ssz_serialize_spec.

Import ListNotations.
Local Open Scope Z_scope.

Lemma int64_repr_inj_uint64 :
  forall x y,
    0 <= x <= uint64_max ->
    0 <= y <= uint64_max ->
    Int64.repr x = Int64.repr y ->
    x = y.
Proof.
  intros x y Hx Hy Heq.
  apply (f_equal Int64.unsigned) in Heq.
  rewrite !Int64.unsigned_repr in Heq by
    (unfold uint64_max, uint64_modulus, Int64.max_unsigned,
       Int64.modulus in *; simpl in *; lia).
  exact Heq.
Qed.

Lemma int64_eq_repr_false_uint64 :
  forall x y,
    0 <= x <= uint64_max ->
    0 <= y <= uint64_max ->
    x <> y ->
    Int64.eq (Int64.repr x) (Int64.repr y) = false.
Proof.
  intros x y Hx Hy Hne.
  apply Int64.eq_false.
  intro Heq.
  apply Hne.
  eapply int64_repr_inj_uint64; eauto.
Qed.

Lemma int64_modu_repr_8 :
  forall z,
    0 <= z <= uint64_max ->
    Int64.modu (Int64.repr z)
      (Int64.repr (Int.unsigned (Int.repr 8))) =
    Int64.repr (z mod 8).
Proof.
  intros z Hrange.
  pose proof (Z.mod_pos_bound z 8 ltac:(lia)) as Hmod.
  unfold Int64.modu.
  change (Int.unsigned (Int.repr 8)) with 8.
  rewrite Int64.unsigned_repr by
    (unfold uint64_max, uint64_modulus, Int64.max_unsigned,
       Int64.modulus in *; simpl in *; lia).
  change (Int64.unsigned (Int64.repr 8)) with 8.
  reflexivity.
Qed.

Ltac solve_impossible_branch :=
  exfalso;
  match goal with
  | Hnull : ?p = nullval, Hptr : isptr ?p |- _ =>
      subst p; inversion Hptr
  | Hnull : nullval = ?p, Hptr : isptr ?p |- _ =>
      subst p; inversion Hptr
  | Hbad : Int64.repr 0 <> Int64.repr 0 |- _ =>
      apply Hbad; reflexivity
  | Hbad : Int64.repr 0 <> Int64.repr (Int.unsigned (Int.repr 0)) |- _ =>
      apply Hbad; reflexivity
  | Hbad : Int.one = Int.zero |- _ =>
      discriminate Hbad
  | Hbad : Int.zero = Int.one |- _ =>
      discriminate Hbad
  | Hbad : Int.one <> Int.zero |- _ =>
      apply Hbad; discriminate
  | Hbad : Int.zero <> Int.zero |- _ =>
      apply Hbad; reflexivity
  | Hbad : Int.repr 0 <> Int.zero |- _ =>
      apply Hbad; reflexivity
  | Hbad : Int.repr 1 = Int.zero |- _ =>
      discriminate Hbad
  | Hbad : Int.repr 1 <> Int.one |- _ =>
      apply Hbad; reflexivity
  | Hbad : nullval <> nullval |- _ =>
      apply Hbad; reflexivity
  | Hbad : 0 <> 0 |- _ =>
      apply Hbad; reflexivity
  | Hbad : false = true |- _ =>
      discriminate Hbad
  | Hbad : true = false |- _ =>
      discriminate Hbad
  end.

Lemma ssz_bits_to_bytes_ok_uint64 :
  forall bit_count,
    0 <= bit_count <= uint64_max ->
    ssz_bits_to_bytes_ok bit_count = true.
Proof.
  intros bit_count Hrange.
  unfold ssz_bits_to_bytes_ok, ssz_bits_to_bytes.
  apply Z.leb_le.
  apply Z.div_le_upper_bound; [lia |].
  unfold uint64_max, uint64_modulus in *.
  simpl in *.
  lia.
Qed.

Lemma ssz_bits_to_bytes_range :
  forall bit_count,
    0 <= bit_count <= uint64_max ->
    0 <= ssz_bits_to_bytes bit_count <= uint64_max.
Proof.
  intros bit_count Hrange.
  split.
  - unfold ssz_bits_to_bytes.
    apply Z.div_pos; lia.
  - unfold ssz_bits_to_bytes.
    apply Z.div_le_upper_bound; [lia |].
    unfold uint64_max, uint64_modulus in *.
    simpl in *.
    lia.
Qed.

Lemma ssz_bits_to_bytes_pos :
  forall bit_count,
    0 < bit_count ->
    0 < ssz_bits_to_bytes bit_count.
Proof.
  intros bit_count Hpos.
  unfold ssz_bits_to_bytes.
  assert (1 <= (bit_count + 7) / 8).
  { apply Z.div_le_lower_bound; lia. }
  lia.
Qed.

Definition bitvector_c_mask (bit_count : Z) : int :=
  Int.zero_ext 8
    (Int.zero_ext 8
      (Int.sub
        (Int.shl (Int.repr 1)
          (Int64.loword
            (Int64.modu (Int64.repr bit_count)
              (Int64.repr (Int.unsigned (Int.repr 8))))))
        (Int.repr 1))).

Lemma bitvector_c_mask_eq :
  forall bit_count,
    0 <= bit_count <= uint64_max ->
    bitvector_c_mask bit_count =
      Int.repr (ssz_byte_mask (bit_count mod 8)).
Proof.
  intros bit_count Hrange.
  pose proof (Z.mod_pos_bound bit_count 8 ltac:(lia)) as Hmod.
  assert (Hpow_bound : 1 <= 2 ^ (bit_count mod 8) <= 2 ^ 7).
  {
    split.
    - pose proof (Z.pow_pos_nonneg 2 (bit_count mod 8)
        ltac:(lia) ltac:(lia)); lia.
    - apply Z.pow_le_mono_r; lia.
  }
  change (2 ^ 7) with 128 in Hpow_bound.
  unfold bitvector_c_mask, ssz_byte_mask.
  unfold Int64.modu, Int64.loword, Int.shl, Int.sub, Int.zero_ext.
  change (Int.unsigned (Int.repr 8)) with 8.
  rewrite (Int64.unsigned_repr bit_count) by
    (unfold uint64_max, uint64_modulus, Int64.max_unsigned,
       Int64.modulus in *; simpl in *; lia).
  change (Int64.unsigned (Int64.repr 8)) with 8.
  rewrite (Int64.unsigned_repr (bit_count mod 8)) by
    (unfold Int64.max_unsigned, Int64.modulus; simpl; lia).
  rewrite (Int.unsigned_repr (bit_count mod 8)) by
    (unfold Int.max_unsigned, Int.modulus; simpl; lia).
  change (Int.unsigned (Int.repr 1)) with 1.
  rewrite Z.shiftl_1_l.
  rewrite (Int.unsigned_repr (2 ^ (bit_count mod 8))) by
    (unfold Int.max_unsigned, Int.modulus; simpl; lia).
  rewrite (Int.unsigned_repr (2 ^ (bit_count mod 8) - 1)) by
    (unfold Int.max_unsigned, Int.modulus; simpl; lia).
  rewrite !Zbits.Zzero_ext_mod by lia.
  change (two_p 8) with 256.
  replace ((2 ^ (bit_count mod 8) - 1) mod 256)
    with (2 ^ (bit_count mod 8) - 1) by
    (symmetry; apply Z.mod_small; lia).
  rewrite Int.unsigned_repr by
    (unfold Int.max_unsigned, Int.modulus; simpl; lia).
  rewrite Z.mod_small by lia.
  reflexivity.
Qed.

Lemma zero_ext_u8_repr :
  forall z,
    0 <= z <= Byte.max_unsigned ->
    Int.zero_ext 8 (Int.repr z) = Int.repr z.
Proof.
  intros z Hz.
  unfold Int.zero_ext.
  rewrite Int.unsigned_repr by
    (unfold Byte.max_unsigned, Int.max_unsigned, Int.modulus in *;
     simpl in *; lia).
  rewrite Zbits.Zzero_ext_mod by lia.
  change (two_p 8) with 256.
  rewrite Z.mod_small by
    (unfold Byte.max_unsigned in Hz; simpl in Hz; lia).
  reflexivity.
Qed.

Lemma ssz_byte_mask_range :
  forall used_bits,
    0 <= used_bits < 8 ->
    0 <= ssz_byte_mask used_bits <= Byte.max_unsigned.
Proof.
  intros used_bits Hused.
  unfold ssz_byte_mask, Byte.max_unsigned.
  simpl.
  assert (Hpow_pos : 0 < 2 ^ used_bits).
  {
    apply Z.pow_pos_nonneg; lia.
  }
  assert (Hpow_bound : 2 ^ used_bits <= 2 ^ 7).
  {
    apply Z.pow_le_mono_r; lia.
  }
  change (2 ^ 7) with 128 in Hpow_bound.
  lia.
Qed.

Lemma ssz_padding_valid_nonzero_mask :
  forall bit_count bytes,
    0 <= bit_count <= uint64_max ->
    bit_count mod 8 <> 0 ->
    ssz_padding_valid bit_count bytes =
      Int.eq
        (Int.and
          (Int.repr
            (Byte.unsigned
              (@Znth byte byte_inhabitant
                (ssz_bits_to_bytes bit_count - 1) bytes)))
          (Int.zero_ext 8 (Int.not (bitvector_c_mask bit_count))))
        Int.zero.
Proof.
  intros bit_count bytes Hrange Hmod_nonzero.
  pose proof (Z.mod_pos_bound bit_count 8 ltac:(lia)) as Hmod_bound.
  pose proof (ssz_byte_mask_range _ Hmod_bound) as Hmask_range.
  unfold ssz_padding_valid.
  replace (bit_count mod 8 =? 0) with false by
    (symmetry; apply Z.eqb_neq; exact Hmod_nonzero).
  rewrite zero_ext_u8_repr by exact Hmask_range.
  rewrite <- bitvector_c_mask_eq by exact Hrange.
  reflexivity.
Qed.

Lemma is_int_u8_vubyte :
  forall b,
    is_int I8 Unsigned (Vubyte b).
Proof.
  intros b.
  unfold Vubyte, is_int.
  rewrite Int.unsigned_repr by
    (pose proof (Byte.unsigned_range_2 b);
     unfold Byte.max_unsigned, Int.max_unsigned, Int.modulus in *;
     simpl in *; lia).
  pose proof (Byte.unsigned_range_2 b).
  lia.
Qed.

Lemma bitvector_padding_typed_false_contradiction :
  forall bit_count bytes,
    0 <= bit_count <= uint64_max ->
    bit_count mod 8 <> 0 ->
    Zlength bytes = ssz_bits_to_bytes bit_count ->
    0 < ssz_bits_to_bytes bit_count ->
    ssz_padding_valid bit_count bytes = false ->
    typed_false tint
      match
        both_int
          (fun n1 n2 : int =>
             Some (bool2val (negb (Int.eq n1 n2))))
          (sem_cast_i2i I32 Unsigned) (sem_cast_i2i I32 Unsigned)
          (force_val
             (sem_and tuchar tuchar
                (Znth (ssz_bits_to_bytes bit_count - 1)
                  (map Vubyte bytes))
                (Vint
                  (Int.zero_ext 8
                    (Int.not (bitvector_c_mask bit_count))))))
          (Vint (Int.repr 0))
      with
      | Some v' => v'
      | None => Vundef
      end ->
    False.
Proof.
  intros bit_count bytes Hrange Hmod_nonzero Hlen Hbyte_pos
    Hpadding Hfalse.
  rewrite Znth_map_Vubyte in Hfalse by lia.
  unfold typed_false, Vubyte, bool2val in Hfalse.
  simpl in Hfalse.
  rewrite (ssz_padding_valid_nonzero_mask
    bit_count bytes Hrange Hmod_nonzero) in Hpadding.
  change (Int.repr 0) with Int.zero in Hfalse.
  destruct (Int.eq
    (Int.and
      (Int.repr
        (Byte.unsigned
          (Znth (ssz_bits_to_bytes bit_count - 1) bytes)))
      (Int.zero_ext 8 (Int.not (bitvector_c_mask bit_count))))
    Int.zero) eqn:Heq.
  - discriminate Hpadding.
  - simpl in Hfalse.
    discriminate.
Qed.

Lemma bitvector_padding_typed_true_contradiction :
  forall bit_count bytes,
    0 <= bit_count <= uint64_max ->
    bit_count mod 8 <> 0 ->
    Zlength bytes = ssz_bits_to_bytes bit_count ->
    0 < ssz_bits_to_bytes bit_count ->
    ssz_padding_valid bit_count bytes = true ->
    typed_true tint
      match
        both_int
          (fun n1 n2 : int =>
             Some (bool2val (negb (Int.eq n1 n2))))
          (sem_cast_i2i I32 Unsigned) (sem_cast_i2i I32 Unsigned)
          (force_val
             (sem_and tuchar tuchar
                (Znth (ssz_bits_to_bytes bit_count - 1)
                  (map Vubyte bytes))
                (Vint
                  (Int.zero_ext 8
                    (Int.not (bitvector_c_mask bit_count))))))
          (Vint (Int.repr 0))
      with
      | Some v' => v'
      | None => Vundef
      end ->
    False.
Proof.
  intros bit_count bytes Hrange Hmod_nonzero Hlen Hbyte_pos
    Hpadding Htrue.
  rewrite Znth_map_Vubyte in Htrue by lia.
  unfold typed_true, Vubyte, bool2val in Htrue.
  simpl in Htrue.
  rewrite (ssz_padding_valid_nonzero_mask
    bit_count bytes Hrange Hmod_nonzero) in Hpadding.
  change (Int.repr 0) with Int.zero in Htrue.
  destruct (Int.eq
    (Int.and
      (Int.repr
        (Byte.unsigned
          (Znth (ssz_bits_to_bytes bit_count - 1) bytes)))
      (Int.zero_ext 8 (Int.not (bitvector_c_mask bit_count))))
    Int.zero) eqn:Heq.
  - simpl in Htrue.
    discriminate.
  - discriminate Hpadding.
Qed.

Ltac solve_bit_count_zero bit_count Hrange Hnonzero :=
  exfalso;
  apply Hnonzero;
  match goal with
  | Heq : Int64.repr bit_count = Int64.zero |- _ =>
      change Int64.zero with (Int64.repr 0) in Heq;
      eapply int64_repr_inj_uint64; eauto;
      split; [lia | unfold uint64_max, uint64_modulus; simpl; lia]
  | Heq : Int64.repr bit_count =
          Int64.repr (Int.unsigned (Int.repr 0)) |- _ =>
      change (Int.unsigned (Int.repr 0)) with 0 in Heq;
      eapply int64_repr_inj_uint64; eauto;
      split; [lia | unfold uint64_max, uint64_modulus; simpl; lia]
  | Heq : Int64.eq (Int64.repr bit_count) (Int64.repr 0) = true |- _ =>
      apply Int64.same_if_eq in Heq;
      eapply int64_repr_inj_uint64; eauto;
      split; [lia | unfold uint64_max, uint64_modulus; simpl; lia]
  end.

Ltac solve_value_len_eq expected :=
  change (Int.unsigned (Int.repr expected)) with expected in *;
  match goal with
  | Heq : Int64.eq (Int64.repr ?len) ?rhs = true |- ?len = expected =>
      apply Int64.same_if_eq in Heq;
      change (Int.unsigned (Int.repr expected)) with expected in Heq;
      eapply int64_repr_inj_uint64; eauto; split;
      [lia | unfold uint64_max, uint64_modulus; simpl; lia]
  | Heq : Int64.repr ?len = ?rhs |- ?len = expected =>
      change (Int.unsigned (Int.repr expected)) with expected in Heq;
      eapply int64_repr_inj_uint64; eauto; split;
      [lia | unfold uint64_max, uint64_modulus; simpl; lia]
  end.

Ltac rewrite_int64_unsigned :=
  rewrite !Int64.unsigned_repr by
    (unfold uint64_max, uint64_modulus, Int64.max_unsigned,
       Int64.modulus in *; simpl in *; lia).

Ltac solve_int64_zero_contradiction z Hz :=
  match goal with
  | Heq : Int64.repr z = Int64.repr 0 |- _ =>
      apply Hz;
      eapply int64_repr_inj_uint64; eauto;
      split; [lia | unfold uint64_max, uint64_modulus; simpl; lia]
  | Heq : Int64.eq (Int64.repr z) (Int64.repr 0) = true |- _ =>
      apply Int64.same_if_eq in Heq;
      apply Hz;
      eapply int64_repr_inj_uint64; eauto;
      split; [lia | unfold uint64_max, uint64_modulus; simpl; lia]
  end.

Lemma body_ssz_internal_prepare_output :
  semax_body Vprog Gprog
    f_ssz_internal_prepare_output ssz_internal_prepare_output_spec.
Proof.
  start_function.
  destruct c as [out | out_len sh_len | out out_len sh_len].
  -
    forward.
    forward_if
      (PROP ()
       LOCAL (temp _err (Vint (Int.repr ssz_err_invalid_argument));
              temp _required (Vlong (Int64.repr required));
              temp _out out; temp _out_cap (Vlong (Int64.repr out_cap));
              temp _out_len nullval)
       SEP (emp)).
    + forward. entailer!.
    + exfalso.
      match goal with
      | Hbad : Int64.repr 0 <> Int64.repr 0 |- _ =>
          apply Hbad; reflexivity
      end.
    + forward.
  -
    forward.
    change (prepare_output_prop
      (Prepare_output_measure_only out_len sh_len))
      with (writable_share sh_len) in H1.
    change (prepare_output_pre
      (Prepare_output_measure_only out_len sh_len))
      with (data_at_ sh_len tulong out_len).
    assert_PROP (isptr out_len) as Hout_len_ptr by entailer!.
    forward_if
      (PROP ()
       LOCAL (temp _err (Vint (Int.repr ssz_success));
              temp _required (Vlong (Int64.repr required));
              temp _out nullval; temp _out_cap (Vlong (Int64.repr out_cap));
              temp _out_len out_len)
       SEP (data_at sh_len tulong
              (Vlong (Int64.repr
                (ssz_internal_prepare_output_len required))) out_len)).
    + exfalso.
      match goal with
      | Hnull : out_len = nullval |- _ =>
          subst out_len; inversion Hout_len_ptr
      end.
    + forward.
      change (prepare_output_out
        (Prepare_output_measure_only out_len sh_len)) with nullval.
      change (prepare_output_out_len
        (Prepare_output_measure_only out_len sh_len)) with out_len.
      forward_if
        (PROP ()
         LOCAL (temp _t'1 (Vint Int.zero);
                temp _err (Vint (Int.repr ssz_success));
                temp _required (Vlong (Int64.repr required));
                temp _out nullval;
                temp _out_cap (Vlong (Int64.repr out_cap));
                temp _out_len out_len)
         SEP (data_at sh_len tulong
                (Vlong (Int64.repr
                  (ssz_internal_prepare_output_len required))) out_len)).
      * exfalso.
        match goal with
        | Hbad : false = true |- _ => discriminate Hbad
        end.
      * forward.
        unfold ssz_success, ssz_internal_prepare_output_len.
        entailer!.
      * forward_if
          (PROP ()
           LOCAL (temp _err (Vint (Int.repr ssz_success));
                  temp _t'1 (Vint Int.zero);
                  temp _required (Vlong (Int64.repr required));
                  temp _out nullval;
                  temp _out_cap (Vlong (Int64.repr out_cap));
                  temp _out_len out_len)
           SEP (data_at sh_len tulong
                  (Vlong (Int64.repr
                    (ssz_internal_prepare_output_len required))) out_len)).
        -- exfalso.
           match goal with
           | Hbad : 0 <> 0 |- _ =>
               apply Hbad; reflexivity
           | Hbad : Int.zero <> Int.zero |- _ =>
               apply Hbad; reflexivity
           end.
        -- forward.
           unfold ssz_success, ssz_internal_prepare_output_len.
           entailer!.
        -- entailer!.
    + forward.
  -
    change (prepare_output_prop
      (Prepare_output_with_out out out_len sh_len))
      with (isptr out /\ writable_share sh_len) in H1.
    destruct H1 as [Hout_ptr Hsh_len].
    forward.
    change (prepare_output_pre
      (Prepare_output_with_out out out_len sh_len))
      with ((data_at_ sh_len tulong out_len * valid_pointer out)%logic).
    Intros.
    assert_PROP (isptr out_len) as Hout_len_ptr by entailer!.
    destruct (out_cap <? required) eqn:Hcap.
    + apply Z.ltb_lt in Hcap.
      forward_if
        (PROP ()
         LOCAL (temp _err (Vint (Int.repr ssz_err_buffer_too_small));
                temp _required (Vlong (Int64.repr required));
                temp _out out; temp _out_cap (Vlong (Int64.repr out_cap));
                temp _out_len out_len)
         SEP (data_at sh_len tulong
                (Vlong (Int64.repr
                  (ssz_internal_prepare_output_len required))) out_len;
              valid_pointer out)).
      * exfalso.
        match goal with
        | Hnull : out_len = nullval |- _ =>
            subst out_len; inversion Hout_len_ptr
        end.
      * forward.
        change (prepare_output_out
          (Prepare_output_with_out out out_len sh_len)) with out.
        change (prepare_output_out_len
          (Prepare_output_with_out out out_len sh_len)) with out_len.
        forward_if
          (PROP ()
           LOCAL (temp _t'1 (Vint Int.one);
                  temp _err (Vint (Int.repr ssz_success));
                  temp _required (Vlong (Int64.repr required));
                  temp _out out;
                  temp _out_cap (Vlong (Int64.repr out_cap));
                  temp _out_len out_len)
           SEP (data_at sh_len tulong
                  (Vlong (Int64.repr
                    (ssz_internal_prepare_output_len required))) out_len;
                valid_pointer out)).
        -- forward.
           entailer!.
           unfold bool2val, Int64.cmpu, Int64.ltu.
           rewrite !Int64.unsigned_repr by
             (unfold uint64_max, uint64_modulus, Int64.max_unsigned,
                Int64.modulus in *; simpl in *; lia).
           rewrite zlt_true by lia.
           reflexivity.
        -- exfalso.
           match goal with
           | Hnull : out = nullval |- _ =>
               subst out; inversion Hout_ptr
           end.
        -- forward_if
            (PROP ()
             LOCAL (temp _err (Vint (Int.repr ssz_err_buffer_too_small));
                    temp _t'1 (Vint Int.one);
                    temp _required (Vlong (Int64.repr required));
                    temp _out out;
                    temp _out_cap (Vlong (Int64.repr out_cap));
                    temp _out_len out_len)
             SEP (data_at sh_len tulong
                    (Vlong (Int64.repr
                      (ssz_internal_prepare_output_len required))) out_len;
                  valid_pointer out)).
           ++ forward.
              unfold ssz_err_buffer_too_small,
                ssz_internal_prepare_output_len.
              entailer!.
           ++ exfalso.
              match goal with
              | Hbad : Int.one = Int.zero |- _ =>
                  discriminate Hbad
              | Hbad : Int.one <> Int.zero |- _ =>
                  apply Hbad; discriminate
              end.
           ++ entailer!.
      * forward.
        unfold prepare_output_result, ssz_internal_prepare_output_result,
          prepare_output_has_out_len, prepare_output_has_out,
          ssz_err_buffer_too_small, ssz_internal_prepare_output_len.
        replace (out_cap <? required) with true by
          (symmetry; apply Z.ltb_lt; lia).
        entailer!.
    + apply Z.ltb_ge in Hcap.
      forward_if
        (PROP ()
         LOCAL (temp _err (Vint (Int.repr ssz_success));
                temp _required (Vlong (Int64.repr required));
                temp _out out; temp _out_cap (Vlong (Int64.repr out_cap));
                temp _out_len out_len)
         SEP (data_at sh_len tulong
                (Vlong (Int64.repr
                  (ssz_internal_prepare_output_len required))) out_len;
              valid_pointer out)).
      * exfalso.
        match goal with
        | Hnull : out_len = nullval |- _ =>
            subst out_len; inversion Hout_len_ptr
        end.
      * forward.
        change (prepare_output_out
          (Prepare_output_with_out out out_len sh_len)) with out.
        change (prepare_output_out_len
          (Prepare_output_with_out out out_len sh_len)) with out_len.
        forward_if
          (PROP ()
           LOCAL (temp _t'1 (Vint Int.zero);
                  temp _err (Vint (Int.repr ssz_success));
                  temp _required (Vlong (Int64.repr required));
                  temp _out out;
                  temp _out_cap (Vlong (Int64.repr out_cap));
                  temp _out_len out_len)
           SEP (data_at sh_len tulong
                  (Vlong (Int64.repr
                    (ssz_internal_prepare_output_len required))) out_len;
                valid_pointer out)).
        -- forward.
           entailer!.
           unfold bool2val, Int64.cmpu, Int64.ltu.
           rewrite !Int64.unsigned_repr by
             (unfold uint64_max, uint64_modulus, Int64.max_unsigned,
                Int64.modulus in *; simpl in *; lia).
           rewrite zlt_false by lia.
           reflexivity.
        -- exfalso.
           match goal with
           | Hnull : out = nullval |- _ =>
               subst out; inversion Hout_ptr
           end.
        -- forward_if
            (PROP ()
             LOCAL (temp _err (Vint (Int.repr ssz_success));
                    temp _t'1 (Vint Int.zero);
                    temp _required (Vlong (Int64.repr required));
                    temp _out out;
                    temp _out_cap (Vlong (Int64.repr out_cap));
                    temp _out_len out_len)
             SEP (data_at sh_len tulong
                    (Vlong (Int64.repr
                      (ssz_internal_prepare_output_len required))) out_len;
                  valid_pointer out)).
           ++ exfalso.
              match goal with
              | Hbad : 0 <> 0 |- _ =>
                  apply Hbad; reflexivity
              | Hbad : Int.zero <> Int.zero |- _ =>
                  apply Hbad; reflexivity
              end.
           ++ forward.
              unfold ssz_success, ssz_internal_prepare_output_len.
              entailer!.
           ++ entailer!.
      * forward.
        unfold prepare_output_result, ssz_internal_prepare_output_result,
          prepare_output_has_out_len, prepare_output_has_out, ssz_success,
          ssz_internal_prepare_output_len.
        replace (out_cap <? required) with false by
          (symmetry; apply Z.ltb_ge; lia).
        entailer!.
Qed.

Lemma body_ssz_serialize_uint8 :
  semax_body Vprog Gprog f_ssz_serialize_uint8 ssz_serialize_uint8_spec.
Proof.
  start_function.
  destruct c as [| out sh].
  - forward.
    forward_if
      (PROP ()
       LOCAL (temp _err (Vint (Int.repr ssz_err_invalid_argument));
              temp _value (Vint (Int.repr value)); temp _out nullval)
       SEP (emp)).
    + forward. entailer!.
    + exfalso.
      match goal with
      | Hbad : Int64.repr 0 <> Int64.repr 0 |- _ =>
          apply Hbad; reflexivity
      end.
    + forward.
  - forward.
    change (serialize_uint8_prop (Serialize_uint8_buffer out sh))
      with (writable_share sh) in H0.
    change (serialize_uint8_out (Serialize_uint8_buffer out sh)) with out.
    change (serialize_uint8_pre (Serialize_uint8_buffer out sh))
      with (field_at_ sh tuchar nil out).
    assert_PROP (isptr out) as Hout_ptr by entailer!.
    forward_if
      (PROP ()
       LOCAL (temp _err (Vint (Int.repr ssz_success));
              temp _value (Vint (Int.repr value)); temp _out out)
       SEP (field_at sh tuchar nil
              (Vubyte (ssz_serialize_uint8_byte value)) out)).
    + apply denote_tc_test_eq_split.
      * apply field_at_valid_ptr0.
        -- apply readable_nonidentity.
           apply writable_readable_share.
           exact H0.
        -- simpl; lia.
        -- reflexivity.
      * apply valid_pointer_zero64.
        reflexivity.
    + exfalso.
      match goal with
      | Hnull : out = nullval |- _ =>
          subst out; inversion Hout_ptr
      end.
    + assert_PROP
        (field_compatible tuchar [] out) as Hout_fc by entailer!.
      assert_PROP
        (force_val
          (sem_add_ptr_int tuchar Signed out (Vint (Int.repr 0))) =
         field_address tuchar [] out)
        as Hout_add_zero.
      {
        entailer!.
        unfold field_address.
        rewrite if_true by exact Hout_fc.
        change (nested_field_offset tuchar []) with 0.
        rewrite isptr_offset_val_zero by exact Hout_ptr.
        reflexivity.
      }
      forward.
      unfold ssz_serialize_uint8_byte, Vubyte.
      rewrite Byte.unsigned_repr by lia.
      entailer!.
      cancel.
    + forward.
Qed.

Lemma body_ssz_serialize_uint16 :
  semax_body Vprog Gprog f_ssz_serialize_uint16 ssz_serialize_uint16_spec.
Proof.
  start_function.
  destruct c as [| out sh].
  - forward.
    forward_if
      (PROP ()
       LOCAL (temp _err (Vint (Int.repr ssz_err_invalid_argument));
              temp _value (Vint (Int.repr value)); temp _out nullval)
       SEP (emp)).
    + forward. entailer!.
    + exfalso.
      match goal with
      | Hbad : Int64.repr 0 <> Int64.repr 0 |- _ =>
          apply Hbad; reflexivity
      end.
    + forward.
  - forward.
    change (serialize_uint16_prop (Serialize_uint16_buffer out sh))
      with (writable_share sh) in H0.
    change (serialize_uint16_out (Serialize_uint16_buffer out sh)) with out.
    change (serialize_uint16_pre (Serialize_uint16_buffer out sh))
      with (data_at_ sh t_ssz_u16_bytes out).
    assert_PROP (isptr out) as Hout_ptr by entailer!.
    forward_if
      (PROP ()
       LOCAL (temp _err (Vint (Int.repr ssz_success));
              temp _value (Vint (Int.repr value)); temp _out out)
       SEP (data_at sh t_ssz_u16_bytes
              (map Vubyte (ssz_serialize_uint16_bytes value)) out)).
    + exfalso.
      match goal with
      | Hnull : out = nullval |- _ =>
          subst out; inversion Hout_ptr
      end.
    + forward_call (out, sh, value).
      {
        unfold t_ssz_u16_bytes.
        entailer!.
      }
    + forward.
Qed.

Lemma body_ssz_serialize_uint32 :
  semax_body Vprog Gprog f_ssz_serialize_uint32 ssz_serialize_uint32_spec.
Proof.
  start_function.
  destruct c as [| out sh].
  - forward.
    forward_if
      (PROP ()
       LOCAL (temp _err (Vint (Int.repr ssz_err_invalid_argument));
              temp _value (Vint (Int.repr value)); temp _out nullval)
       SEP (emp)).
    + forward. entailer!.
    + exfalso.
      match goal with
      | Hbad : Int64.repr 0 <> Int64.repr 0 |- _ =>
          apply Hbad; reflexivity
      end.
    + forward.
  - forward.
    change (serialize_uint32_prop (Serialize_uint32_buffer out sh))
      with (writable_share sh) in H0.
    change (serialize_uint32_out (Serialize_uint32_buffer out sh)) with out.
    change (serialize_uint32_pre (Serialize_uint32_buffer out sh))
      with (data_at_ sh t_ssz_u32_bytes out).
    assert_PROP (isptr out) as Hout_ptr by entailer!.
    forward_if
      (PROP ()
       LOCAL (temp _err (Vint (Int.repr ssz_success));
              temp _value (Vint (Int.repr value)); temp _out out)
       SEP (data_at sh t_ssz_u32_bytes
              (map Vubyte (ssz_serialize_uint32_bytes value)) out)).
    + exfalso.
      match goal with
      | Hnull : out = nullval |- _ =>
          subst out; inversion Hout_ptr
      end.
    + forward_call (out, sh, value).
      {
        unfold t_ssz_u32_bytes.
        entailer!.
      }
    + forward.
Qed.

Lemma body_ssz_serialize_uint64 :
  semax_body Vprog Gprog f_ssz_serialize_uint64 ssz_serialize_uint64_spec.
Proof.
  start_function.
  destruct c as [| out sh].
  - forward.
    forward_if
      (PROP ()
       LOCAL (temp _err (Vint (Int.repr ssz_err_invalid_argument));
              temp _value (Vlong (Int64.repr value)); temp _out nullval)
       SEP (emp)).
    + forward. entailer!.
    + exfalso.
      match goal with
      | Hbad : Int64.repr 0 <> Int64.repr 0 |- _ =>
          apply Hbad; reflexivity
      end.
    + forward.
  - forward.
    change (serialize_uint64_prop (Serialize_uint64_buffer out sh))
      with (writable_share sh) in H0.
    change (serialize_uint64_out (Serialize_uint64_buffer out sh)) with out.
    change (serialize_uint64_pre (Serialize_uint64_buffer out sh))
      with (ssz_u64_le_data_at_ sh out).
    unfold ssz_u64_le_data_at_; Intros.
    assert_PROP (field_compatible tuchar nil out) as FC0 by entailer!.
    pose proof (field_compatible_isptr _ _ _ FC0) as Hout_ptr.
    forward_if
      (PROP ()
       LOCAL (temp _err (Vint (Int.repr ssz_success));
              temp _value (Vlong (Int64.repr value)); temp _out out)
       SEP (ssz_u64_le_data_at sh
              (map Vubyte (ssz_serialize_uint64_bytes value)) out)).
    + apply denote_tc_test_eq_split.
      * repeat eapply sepcon_valid_pointer1.
        apply field_at_valid_ptr0.
        -- apply readable_nonidentity.
           apply writable_readable_share.
           exact H0.
        -- simpl; lia.
        -- reflexivity.
      * apply valid_pointer_zero64.
        reflexivity.
    + exfalso.
      match goal with
      | Hnull : out = nullval |- _ =>
          subst out; inversion Hout_ptr
      end.
    + forward_call (out, sh, value).
      {
        unfold ssz_u64_le_data_at_.
        entailer!.
      }
      unfold ssz_u64_le_data_at, ssz_serialize_uint64_bytes,
        ssz_u64_le_bytes, ssz_success.
      entailer!.
      cancel.
    + forward.
Qed.

Lemma body_ssz_serialize_uint128 :
  semax_body Vprog Gprog f_ssz_serialize_uint128 ssz_serialize_uint128_spec.
Proof.
  start_function.
  destruct c as [out | value | value out
    | value out sh_value sh_out bytes].
  -
    forward.
    forward_if
      (PROP ()
       LOCAL (temp _t'1 (Vint Int.one);
              temp _err (Vint (Int.repr ssz_success));
              temp _value nullval;
              temp _value_len (Vlong (Int64.repr value_len));
              temp _out out)
       SEP (emp)).
    + forward. entailer!.
    + solve_impossible_branch.
    + forward_if
        (PROP ()
         LOCAL (temp _err (Vint (Int.repr ssz_err_invalid_argument));
                temp _t'1 (Vint Int.one);
                temp _value nullval;
                temp _value_len (Vlong (Int64.repr value_len));
                temp _out out)
         SEP (emp)).
      * forward. entailer!.
      * solve_impossible_branch.
      * forward.
  -
    forward.
    change (serialize_uint128_prop (Serialize_uint128_out_null value))
      with (isptr value) in H0.
    change (serialize_uint128_pre (Serialize_uint128_out_null value))
      with (valid_pointer value).
    forward_if
      (PROP ()
       LOCAL (temp _t'1 (Vint Int.one);
              temp _err (Vint (Int.repr ssz_success));
              temp _value value;
              temp _value_len (Vlong (Int64.repr value_len));
              temp _out nullval)
       SEP (valid_pointer value)).
    + solve_impossible_branch.
    + forward. entailer!.
    + forward_if
        (PROP ()
         LOCAL (temp _err (Vint (Int.repr ssz_err_invalid_argument));
                temp _t'1 (Vint Int.one);
                temp _value value;
                temp _value_len (Vlong (Int64.repr value_len));
                temp _out nullval)
         SEP (valid_pointer value)).
      * forward. entailer!.
      * solve_impossible_branch.
      * forward.
  -
    forward.
    change (serialize_uint128_prop
      (Serialize_uint128_bad_len value out))
      with (isptr value /\ isptr out) in H0.
    destruct H0 as [Hvalue_ptr Hout_ptr].
    change (serialize_uint128_pre
      (Serialize_uint128_bad_len value out))
      with ((valid_pointer value * valid_pointer out)%logic).
    Intros.
    forward_if
      (PROP ()
       LOCAL (temp _t'1 (Vint Int.zero);
              temp _err (Vint (Int.repr ssz_success));
              temp _value value;
              temp _value_len (Vlong (Int64.repr value_len));
              temp _out out)
       SEP (valid_pointer value; valid_pointer out)).
    + solve_impossible_branch.
    + forward.
      entailer!.
      destruct out; inversion Hout_ptr; reflexivity.
    + forward_if
        (PROP ()
         LOCAL (temp _err (Vint (Int.repr ssz_err_encoding_invalid));
                temp _t'1 (Vint Int.zero);
                temp _value value;
                temp _value_len (Vlong (Int64.repr value_len));
                temp _out out)
         SEP (valid_pointer value; valid_pointer out)).
      * solve_impossible_branch.
      * forward_if
          (PROP ()
           LOCAL (temp _err (Vint (Int.repr ssz_err_encoding_invalid));
                  temp _t'1 (Vint Int.zero);
                  temp _value value;
                  temp _value_len (Vlong (Int64.repr value_len));
                  temp _out out)
           SEP (valid_pointer value; valid_pointer out)).
        -- forward.
           unfold serialize_uint128_result,
             ssz_serialize_fixed_bytes_result,
             serialize_uint128_has_value, serialize_uint128_has_out,
             ssz_err_encoding_invalid.
           entailer!.
        -- exfalso.
           apply H1.
           solve_value_len_eq 16.
      * forward.
        unfold serialize_uint128_result,
          ssz_serialize_fixed_bytes_result,
          serialize_uint128_has_value, serialize_uint128_has_out,
          ssz_err_encoding_invalid.
        entailer!.
        replace (value_len =? 16) with false by
          (symmetry; apply Z.eqb_neq; exact H1).
        reflexivity.
  -
    forward.
    change (serialize_uint128_prop
      (Serialize_uint128_buffer value out sh_value sh_out bytes))
      with (readable_share sh_value /\
            writable_share sh_out /\ Zlength bytes = 16) in H0.
    destruct H0 as [Hsh_value [Hsh_out Hbytes_len]].
    change (serialize_uint128_len_prop
      (Serialize_uint128_buffer value out sh_value sh_out bytes) value_len)
      with (value_len = 16) in H1.
    change (serialize_uint128_pre
      (Serialize_uint128_buffer value out sh_value sh_out bytes))
      with ((data_at sh_value t_ssz_u128_bytes (map Vubyte bytes) value *
             data_at_ sh_out t_ssz_u128_bytes out)%logic).
    Intros.
    assert_PROP (isptr value) as Hvalue_ptr by entailer!.
    assert_PROP (isptr out) as Hout_ptr by entailer!.
    forward_if
      (PROP ()
       LOCAL (temp _t'1 (Vint Int.zero);
              temp _err (Vint (Int.repr ssz_success));
              temp _value value;
              temp _value_len (Vlong (Int64.repr value_len));
              temp _out out)
       SEP (data_at sh_value t_ssz_u128_bytes (map Vubyte bytes) value;
            data_at_ sh_out t_ssz_u128_bytes out)).
    + solve_impossible_branch.
    + forward.
      entailer!.
      destruct out; inversion Hout_ptr; reflexivity.
    + forward_if
        (PROP ()
         LOCAL (temp _err (Vint (Int.repr ssz_success));
                temp _t'1 (Vint Int.zero);
                temp _value value;
                temp _value_len (Vlong (Int64.repr value_len));
                temp _out out)
         SEP (data_at sh_out t_ssz_u128_bytes
                (map Vubyte (ssz_serialize_uint128_bytes bytes)) out;
              data_at sh_value t_ssz_u128_bytes
                (map Vubyte bytes) value)).
      * solve_impossible_branch.
      * forward_if
          (PROP ()
           LOCAL (temp _err (Vint (Int.repr ssz_success));
                  temp _t'1 (Vint Int.zero);
                  temp _value value;
                  temp _value_len (Vlong (Int64.repr value_len));
                  temp _out out)
           SEP (data_at sh_out t_ssz_u128_bytes
                  (map Vubyte (ssz_serialize_uint128_bytes bytes)) out;
                data_at sh_value t_ssz_u128_bytes
                  (map Vubyte bytes) value)).
        -- exfalso.
           subst value_len.
           change (Int.unsigned (Int.repr 16)) with 16 in *.
           match goal with
           | Hne : Int64.eq (Int64.repr 16)
                    (Int64.repr 16) = false |- _ =>
               rewrite Int64.eq_true in Hne; discriminate
           | Hne : Int64.repr 16 <> Int64.repr 16 |- _ =>
               apply Hne; reflexivity
           end.
        -- subst value_len.
           forward_call
             (16, map Vubyte bytes, out, value, sh_out, sh_value).
           {
             unfold t_ssz_u128_bytes.
             rewrite Zlength_map.
             exact Hbytes_len.
           }
           unfold ssz_serialize_uint128_bytes.
           entailer!.
      * forward.
Qed.

Lemma body_ssz_serialize_uint256 :
  semax_body Vprog Gprog f_ssz_serialize_uint256 ssz_serialize_uint256_spec.
Proof.
  start_function.
  destruct c as [out | value | value out
    | value out sh_value sh_out bytes].
  -
    forward.
    forward_if
      (PROP ()
       LOCAL (temp _t'1 (Vint Int.one);
              temp _err (Vint (Int.repr ssz_success));
              temp _value nullval;
              temp _value_len (Vlong (Int64.repr value_len));
              temp _out out)
       SEP (emp)).
    + forward. entailer!.
    + solve_impossible_branch.
    + forward_if
        (PROP ()
         LOCAL (temp _err (Vint (Int.repr ssz_err_invalid_argument));
                temp _t'1 (Vint Int.one);
                temp _value nullval;
                temp _value_len (Vlong (Int64.repr value_len));
                temp _out out)
         SEP (emp)).
      * forward. entailer!.
      * solve_impossible_branch.
      * forward.
  -
    forward.
    change (serialize_uint256_prop (Serialize_uint256_out_null value))
      with (isptr value) in H0.
    change (serialize_uint256_pre (Serialize_uint256_out_null value))
      with (valid_pointer value).
    forward_if
      (PROP ()
       LOCAL (temp _t'1 (Vint Int.one);
              temp _err (Vint (Int.repr ssz_success));
              temp _value value;
              temp _value_len (Vlong (Int64.repr value_len));
              temp _out nullval)
       SEP (valid_pointer value)).
    + solve_impossible_branch.
    + forward. entailer!.
    + forward_if
        (PROP ()
         LOCAL (temp _err (Vint (Int.repr ssz_err_invalid_argument));
                temp _t'1 (Vint Int.one);
                temp _value value;
                temp _value_len (Vlong (Int64.repr value_len));
                temp _out nullval)
         SEP (valid_pointer value)).
      * forward. entailer!.
      * solve_impossible_branch.
      * forward.
  -
    forward.
    change (serialize_uint256_prop
      (Serialize_uint256_bad_len value out))
      with (isptr value /\ isptr out) in H0.
    destruct H0 as [Hvalue_ptr Hout_ptr].
    change (serialize_uint256_pre
      (Serialize_uint256_bad_len value out))
      with ((valid_pointer value * valid_pointer out)%logic).
    Intros.
    forward_if
      (PROP ()
       LOCAL (temp _t'1 (Vint Int.zero);
              temp _err (Vint (Int.repr ssz_success));
              temp _value value;
              temp _value_len (Vlong (Int64.repr value_len));
              temp _out out)
       SEP (valid_pointer value; valid_pointer out)).
    + solve_impossible_branch.
    + forward.
      entailer!.
      destruct out; inversion Hout_ptr; reflexivity.
    + forward_if
        (PROP ()
         LOCAL (temp _err (Vint (Int.repr ssz_err_encoding_invalid));
                temp _t'1 (Vint Int.zero);
                temp _value value;
                temp _value_len (Vlong (Int64.repr value_len));
                temp _out out)
         SEP (valid_pointer value; valid_pointer out)).
      * solve_impossible_branch.
      * forward_if
          (PROP ()
           LOCAL (temp _err (Vint (Int.repr ssz_err_encoding_invalid));
                  temp _t'1 (Vint Int.zero);
                  temp _value value;
                  temp _value_len (Vlong (Int64.repr value_len));
                  temp _out out)
           SEP (valid_pointer value; valid_pointer out)).
        -- forward.
           unfold serialize_uint256_result,
             ssz_serialize_fixed_bytes_result,
             serialize_uint256_has_value, serialize_uint256_has_out,
             ssz_err_encoding_invalid.
           entailer!.
        -- exfalso.
           apply H1.
           solve_value_len_eq 32.
      * forward.
        unfold serialize_uint256_result,
          ssz_serialize_fixed_bytes_result,
          serialize_uint256_has_value, serialize_uint256_has_out,
          ssz_err_encoding_invalid.
        entailer!.
        replace (value_len =? 32) with false by
          (symmetry; apply Z.eqb_neq; exact H1).
        reflexivity.
  -
    forward.
    change (serialize_uint256_prop
      (Serialize_uint256_buffer value out sh_value sh_out bytes))
      with (readable_share sh_value /\
            writable_share sh_out /\ Zlength bytes = 32) in H0.
    destruct H0 as [Hsh_value [Hsh_out Hbytes_len]].
    change (serialize_uint256_len_prop
      (Serialize_uint256_buffer value out sh_value sh_out bytes) value_len)
      with (value_len = 32) in H1.
    change (serialize_uint256_pre
      (Serialize_uint256_buffer value out sh_value sh_out bytes))
      with ((data_at sh_value t_ssz_u256_bytes (map Vubyte bytes) value *
             data_at_ sh_out t_ssz_u256_bytes out)%logic).
    Intros.
    assert_PROP (isptr value) as Hvalue_ptr by entailer!.
    assert_PROP (isptr out) as Hout_ptr by entailer!.
    forward_if
      (PROP ()
       LOCAL (temp _t'1 (Vint Int.zero);
              temp _err (Vint (Int.repr ssz_success));
              temp _value value;
              temp _value_len (Vlong (Int64.repr value_len));
              temp _out out)
       SEP (data_at sh_value t_ssz_u256_bytes (map Vubyte bytes) value;
            data_at_ sh_out t_ssz_u256_bytes out)).
    + solve_impossible_branch.
    + forward.
      entailer!.
      destruct out; inversion Hout_ptr; reflexivity.
    + forward_if
        (PROP ()
         LOCAL (temp _err (Vint (Int.repr ssz_success));
                temp _t'1 (Vint Int.zero);
                temp _value value;
                temp _value_len (Vlong (Int64.repr value_len));
                temp _out out)
         SEP (data_at sh_out t_ssz_u256_bytes
                (map Vubyte (ssz_serialize_uint256_bytes bytes)) out;
              data_at sh_value t_ssz_u256_bytes
                (map Vubyte bytes) value)).
      * solve_impossible_branch.
      * forward_if
          (PROP ()
           LOCAL (temp _err (Vint (Int.repr ssz_success));
                  temp _t'1 (Vint Int.zero);
                  temp _value value;
                  temp _value_len (Vlong (Int64.repr value_len));
                  temp _out out)
           SEP (data_at sh_out t_ssz_u256_bytes
                  (map Vubyte (ssz_serialize_uint256_bytes bytes)) out;
                data_at sh_value t_ssz_u256_bytes
                  (map Vubyte bytes) value)).
        -- exfalso.
           subst value_len.
           change (Int.unsigned (Int.repr 32)) with 32 in *.
           match goal with
           | Hne : Int64.eq (Int64.repr 32)
                    (Int64.repr 32) = false |- _ =>
               rewrite Int64.eq_true in Hne; discriminate
           | Hne : Int64.repr 32 <> Int64.repr 32 |- _ =>
               apply Hne; reflexivity
           end.
        -- subst value_len.
           forward_call
             (32, map Vubyte bytes, out, value, sh_out, sh_value).
           {
             unfold t_ssz_u256_bytes.
             rewrite Zlength_map.
             exact Hbytes_len.
           }
           unfold ssz_serialize_uint256_bytes.
           entailer!.
      * forward.
Qed.

Lemma body_ssz_serialize_boolean :
  semax_body Vprog Gprog f_ssz_serialize_boolean ssz_serialize_boolean_spec.
Proof.
  start_function.
  destruct c as [| out sh].
  -
    forward.
    forward_if
      (PROP ()
       LOCAL (temp _err (Vint (Int.repr ssz_err_invalid_argument));
              temp _value (Vint (Int.repr value)); temp _out nullval)
       SEP (emp)).
    + forward. entailer!.
    + solve_impossible_branch.
    + forward.
  -
    forward.
    change (serialize_boolean_prop (Serialize_boolean_buffer out sh))
      with (writable_share sh) in H0.
    change (serialize_boolean_out (Serialize_boolean_buffer out sh)) with out.
    change (serialize_boolean_pre (Serialize_boolean_buffer out sh))
      with (field_at_ sh tuchar nil out).
    assert_PROP (isptr out) as Hout_ptr by entailer!.
    forward_if
      (PROP ()
       LOCAL (temp _err
                (Vint (Int.repr
                  (if value <=? 1
                   then ssz_success
                   else ssz_err_encoding_invalid)));
              temp _value (Vint (Int.repr value)); temp _out out)
       SEP (serialize_boolean_post
              (Serialize_boolean_buffer out sh) value)).
    + apply denote_tc_test_eq_split.
      * apply field_at_valid_ptr0.
        -- apply readable_nonidentity.
           apply writable_readable_share.
           exact H0.
        -- simpl; lia.
        -- reflexivity.
      * apply valid_pointer_zero64.
        reflexivity.
    + solve_impossible_branch.
    + destruct (value <=? 1) eqn:Hvalue.
      * pose proof Hvalue as Hvalue_eq.
        apply Z.leb_le in Hvalue.
        forward_if
          (PROP ()
           LOCAL (temp _err (Vint (Int.repr ssz_success));
                  temp _value (Vint (Int.repr value)); temp _out out)
           SEP (serialize_boolean_post
                  (Serialize_boolean_buffer out sh) value)).
        -- exfalso.
           match goal with
           | Hgt : Int.ltu (Int.repr 1) (Int.repr value) = true |- _ =>
               apply Int.ltu_inv in Hgt;
               rewrite !Int.unsigned_repr in Hgt by
                 (unfold Byte.max_unsigned in *; simpl in *; lia);
               lia
           | Hgt : (value > 1)%Z |- _ => lia
           | Hgt : (1 < value)%Z |- _ => lia
           end.
        -- assert_PROP
             (field_compatible tuchar [] out) as Hout_fc by entailer!.
           assert_PROP
             (force_val
               (sem_add_ptr_int tuchar Signed out (Vint (Int.repr 0))) =
              field_address tuchar [] out)
             as Hout_add_zero.
           {
             entailer!.
             unfold field_address.
             rewrite if_true by exact Hout_fc.
             change (nested_field_offset tuchar []) with 0.
             rewrite isptr_offset_val_zero by exact Hout_ptr.
             reflexivity.
           }
           forward.
           unfold serialize_boolean_post,
             ssz_serialize_boolean_byte, Vubyte.
           rewrite Hvalue_eq.
           rewrite Byte.unsigned_repr by lia.
           entailer!.
           cancel.
      * pose proof Hvalue as Hvalue_eq.
        apply Z.leb_gt in Hvalue.
        forward_if
          (PROP ()
           LOCAL (temp _err (Vint (Int.repr ssz_err_encoding_invalid));
                  temp _value (Vint (Int.repr value)); temp _out out)
           SEP (serialize_boolean_post
                  (Serialize_boolean_buffer out sh) value)).
        -- forward.
           unfold serialize_boolean_post.
           rewrite Hvalue_eq.
           entailer!.
        -- exfalso.
           match goal with
           | Hle : value <= 1 |- _ => lia
           | Hle : (1 >= value)%Z |- _ => lia
           end.
    + forward.
Qed.

Lemma body_ssz_serialize_bitvector :
  semax_body Vprog Gprog
    f_ssz_serialize_bitvector ssz_serialize_bitvector_spec.
Proof.
  start_function.
  destruct c as
    [bits_le out out_len bits_le_len out_cap
    | bits_le out out_len bits_le_len bit_count out_cap
    | out out_len bits_le_len bit_count out_cap
    | bits_le out out_len bits_le_len bit_count out_cap
    | bits_le out out_len sh_bits bytes bits_le_len bit_count out_cap
    | bits_le sh_bits bytes bits_le_len bit_count out_cap out_case].
  -
    forward.
    forward.
    forward_if
      (PROP ()
       LOCAL (temp _err (Vint (Int.repr ssz_err_schema_invalid));
              lvar _byte_count tulong v_byte_count;
              temp _bits_le bits_le;
              temp _bits_le_len (Vlong (Int64.repr bits_le_len));
              temp _bit_count (Vlong (Int64.repr 0));
              temp _out out;
              temp _out_cap (Vlong (Int64.repr out_cap));
              temp _out_len out_len)
       SEP (data_at Tsh tulong (Vlong (Int64.repr 0)) v_byte_count)).
    + forward. entailer!.
    + solve_impossible_branch.
    + forward. entailer!.
  -
    change (serialize_bitvector_prop
      (Serialize_bitvector_overflow bits_le out out_len bits_le_len
        bit_count out_cap)) in H.
    destruct H as (_ & Hbit & _ & Hok & _).
    rewrite (ssz_bits_to_bytes_ok_uint64 _ Hbit) in Hok.
    discriminate.
  -
    change (serialize_bitvector_prop
      (Serialize_bitvector_bits_null out out_len bits_le_len bit_count
        out_cap)) in H.
    destruct H as (Hlen & Hbit & Hnz & Hok & Hcap & Hout & Houtlen).
    pose proof (ssz_bits_to_bytes_range _ Hbit) as Hbyte_range.
    forward.
    forward.
    forward_if
      (PROP ()
       LOCAL (temp _err (Vint (Int.repr ssz_err_invalid_argument));
              lvar _byte_count tulong v_byte_count;
              temp _bits_le nullval;
              temp _bits_le_len (Vlong (Int64.repr bits_le_len));
              temp _bit_count (Vlong (Int64.repr bit_count));
              temp _out out;
              temp _out_cap (Vlong (Int64.repr out_cap));
              temp _out_len out_len)
       SEP (data_at Tsh tulong
              (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)))
              v_byte_count)).
    + solve_bit_count_zero bit_count Hbit Hnz.
    + forward_call (bit_count, v_byte_count, Tsh).
      rewrite Hok.
      forward_if
        (PROP ()
         LOCAL (temp _err (Vint (Int.repr ssz_err_invalid_argument));
                temp _t'4 (Vint Int.one);
                lvar _byte_count tulong v_byte_count;
                temp _bits_le nullval;
                temp _bits_le_len (Vlong (Int64.repr bits_le_len));
                temp _bit_count (Vlong (Int64.repr bit_count));
                temp _out out;
                temp _out_cap (Vlong (Int64.repr out_cap));
                temp _out_len out_len)
         SEP (data_at Tsh tulong
                (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)))
                v_byte_count)).
      * solve_impossible_branch.
      * forward_if
          (PROP ()
           LOCAL (temp _t'3 (Vint Int.one);
                  temp _t'4 (Vint Int.one);
                  temp _err (Vint (Int.repr 0));
                  lvar _byte_count tulong v_byte_count;
                  temp _bits_le nullval;
                  temp _bits_le_len (Vlong (Int64.repr bits_le_len));
                  temp _bit_count (Vlong (Int64.repr bit_count));
                  temp _out out;
                  temp _out_cap (Vlong (Int64.repr out_cap));
                  temp _out_len out_len)
           SEP (data_at Tsh tulong
                  (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)))
                  v_byte_count)).
        -- forward. entailer!.
        -- solve_impossible_branch.
        -- forward_if
            (PROP ()
             LOCAL (temp _err (Vint (Int.repr ssz_err_invalid_argument));
                    temp _t'3 (Vint Int.one);
                    temp _t'4 (Vint Int.one);
                    lvar _byte_count tulong v_byte_count;
                    temp _bits_le nullval;
                    temp _bits_le_len (Vlong (Int64.repr bits_le_len));
                    temp _bit_count (Vlong (Int64.repr bit_count));
                    temp _out out;
                    temp _out_cap (Vlong (Int64.repr out_cap));
                    temp _out_len out_len)
             SEP (data_at Tsh tulong
                    (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)))
                    v_byte_count)).
           ++ forward. entailer!.
           ++ solve_impossible_branch.
           ++ entailer!.
      * entailer!.
    + forward.
      unfold serialize_bitvector_result,
        ssz_serialize_bitvector_result.
      simpl.
      replace (bit_count =? 0) with false by
        (symmetry; apply Z.eqb_neq; exact Hnz).
      rewrite Hok.
      entailer!.
  -
    change (serialize_bitvector_prop
      (Serialize_bitvector_bits_short bits_le out out_len bits_le_len
        bit_count out_cap)) in H.
    destruct H as
      (Hlen & Hbit & Hnz & Hok & Hshort & Hcap & Hbits_ptr & Hout &
       Houtlen).
    change (serialize_bitvector_pre
      (Serialize_bitvector_bits_short bits_le out out_len bits_le_len
        bit_count out_cap)) with (valid_pointer bits_le).
    change (serialize_bitvector_post
      (Serialize_bitvector_bits_short bits_le out out_len bits_le_len
        bit_count out_cap)) with (valid_pointer bits_le).
    pose proof (ssz_bits_to_bytes_range _ Hbit) as Hbyte_range.
    forward.
    forward.
    forward_if
      (PROP ()
       LOCAL (temp _err (Vint (Int.repr ssz_err_invalid_argument));
              lvar _byte_count tulong v_byte_count;
              temp _bits_le bits_le;
              temp _bits_le_len (Vlong (Int64.repr bits_le_len));
              temp _bit_count (Vlong (Int64.repr bit_count));
              temp _out out;
              temp _out_cap (Vlong (Int64.repr out_cap));
              temp _out_len out_len)
       SEP (data_at Tsh tulong
              (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)))
              v_byte_count;
            valid_pointer bits_le)).
    + solve_bit_count_zero bit_count Hbit Hnz.
    + forward_call (bit_count, v_byte_count, Tsh).
      rewrite Hok.
      forward_if
        (PROP ()
         LOCAL (temp _err (Vint (Int.repr ssz_err_invalid_argument));
                temp _t'4 (Vint Int.one);
                lvar _byte_count tulong v_byte_count;
                temp _bits_le bits_le;
                temp _bits_le_len (Vlong (Int64.repr bits_le_len));
                temp _bit_count (Vlong (Int64.repr bit_count));
                temp _out out;
                temp _out_cap (Vlong (Int64.repr out_cap));
                temp _out_len out_len)
         SEP (data_at Tsh tulong
                (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)))
                v_byte_count;
              valid_pointer bits_le)).
      * solve_impossible_branch.
      * forward_if
          (PROP ()
           LOCAL (temp _t'3 (Vint Int.one);
                  temp _t'9
                    (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)));
                  temp _t'4 (Vint Int.one);
                  temp _err (Vint (Int.repr 0));
                  lvar _byte_count tulong v_byte_count;
                  temp _bits_le bits_le;
                  temp _bits_le_len (Vlong (Int64.repr bits_le_len));
                  temp _bit_count (Vlong (Int64.repr bit_count));
                  temp _out out;
                  temp _out_cap (Vlong (Int64.repr out_cap));
                  temp _out_len out_len)
           SEP (data_at Tsh tulong
                  (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)))
                  v_byte_count;
                valid_pointer bits_le)).
        -- solve_impossible_branch.
        -- forward.
           forward.
           entailer!.
           unfold bool2val, Int64.cmpu, Int64.ltu.
           rewrite !Int64.unsigned_repr by
             (unfold uint64_max, uint64_modulus, Int64.max_unsigned,
                Int64.modulus in *; simpl in *; lia).
           change (serialize_bitvector_bits_le_len
             (Serialize_bitvector_bits_short bits_le out out_len bits_le_len
                bit_count out_cap)) with bits_le_len.
           replace
             (if zlt bits_le_len (ssz_bits_to_bytes bit_count)
              then true else false)
             with true by
             (destruct (zlt bits_le_len
                (ssz_bits_to_bytes bit_count)); [reflexivity | lia]).
           reflexivity.
        -- forward_if
            (PROP ()
             LOCAL (temp _err (Vint (Int.repr ssz_err_invalid_argument));
                    temp _t'3 (Vint Int.one);
                    temp _t'9
                      (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)));
                    temp _t'4 (Vint Int.one);
                    lvar _byte_count tulong v_byte_count;
                    temp _bits_le bits_le;
                    temp _bits_le_len (Vlong (Int64.repr bits_le_len));
                    temp _bit_count (Vlong (Int64.repr bit_count));
                    temp _out out;
                    temp _out_cap (Vlong (Int64.repr out_cap));
                    temp _out_len out_len)
             SEP (data_at Tsh tulong
                    (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)))
                    v_byte_count;
                  valid_pointer bits_le)).
           ++ forward. entailer!.
           ++ solve_impossible_branch.
           ++ entailer!.
      * entailer!.
    + forward.
      unfold serialize_bitvector_result,
        ssz_serialize_bitvector_result.
      simpl.
      replace (bit_count =? 0) with false by
        (symmetry; apply Z.eqb_neq; exact Hnz).
      rewrite Hok.
      replace (bits_le_len <? ssz_bits_to_bytes bit_count) with true by
        (symmetry; apply Z.ltb_lt; exact Hshort).
      entailer!.
  -
    change (serialize_bitvector_prop
      (Serialize_bitvector_padding_invalid bits_le out out_len sh_bits
        bytes bits_le_len bit_count out_cap)) in H.
    destruct H as
      (Hlen & Hbit & Hnz & Hok & Hshort & Hmod_nonzero & Hpadding &
       Hbytes_len & Hsh_bits & Hcap & Hout & Houtlen).
    change (serialize_bitvector_pre
      (Serialize_bitvector_padding_invalid bits_le out out_len sh_bits
        bytes bits_le_len bit_count out_cap))
      with (data_at sh_bits
        (tarray tuchar (ssz_bits_to_bytes bit_count))
        (map Vubyte bytes) bits_le).
    change (serialize_bitvector_post
      (Serialize_bitvector_padding_invalid bits_le out out_len sh_bits
        bytes bits_le_len bit_count out_cap))
      with (data_at sh_bits
        (tarray tuchar (ssz_bits_to_bytes bit_count))
        (map Vubyte bytes) bits_le).
    pose proof (ssz_bits_to_bytes_range _ Hbit) as Hbyte_range.
    assert (Hbit_pos : 0 < bit_count) by lia.
    pose proof (ssz_bits_to_bytes_pos _ Hbit_pos) as Hbyte_pos.
    assert (Hlast_index :
      0 <= ssz_bits_to_bytes bit_count - 1 < Zlength bytes) by lia.
    pose proof (Z.mod_pos_bound bit_count 8 ltac:(lia)) as Hmod_bound.
    forward.
    forward.
    assert_PROP (isptr bits_le) as Hbits_ptr by entailer!.
    forward_if
      (PROP ()
       LOCAL (temp _err (Vint (Int.repr ssz_err_encoding_invalid));
              lvar _byte_count tulong v_byte_count;
              temp _bits_le bits_le;
              temp _bits_le_len (Vlong (Int64.repr bits_le_len));
              temp _bit_count (Vlong (Int64.repr bit_count));
              temp _out out;
              temp _out_cap (Vlong (Int64.repr out_cap));
              temp _out_len out_len)
       SEP (data_at Tsh tulong
              (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)))
              v_byte_count;
            data_at sh_bits
              (tarray tuchar (ssz_bits_to_bytes bit_count))
              (map Vubyte bytes) bits_le)).
    + solve_bit_count_zero bit_count Hbit Hnz.
    + forward_call (bit_count, v_byte_count, Tsh).
      rewrite Hok.
      forward_if
        (PROP ()
         LOCAL (temp _err (Vint (Int.repr ssz_err_encoding_invalid));
                temp _t'4 (Vint Int.one);
                lvar _byte_count tulong v_byte_count;
                temp _bits_le bits_le;
                temp _bits_le_len (Vlong (Int64.repr bits_le_len));
                temp _bit_count (Vlong (Int64.repr bit_count));
                temp _out out;
                temp _out_cap (Vlong (Int64.repr out_cap));
                temp _out_len out_len)
         SEP (data_at Tsh tulong
                (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)))
                v_byte_count;
              data_at sh_bits
                (tarray tuchar (ssz_bits_to_bytes bit_count))
                (map Vubyte bytes) bits_le)).
      * solve_impossible_branch.
      * forward_if
          (PROP ()
           LOCAL (temp _t'3 (Vint Int.zero);
                  temp _t'9
                    (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)));
                  temp _t'4 (Vint Int.one);
                  temp _err (Vint (Int.repr 0));
                  lvar _byte_count tulong v_byte_count;
                  temp _bits_le bits_le;
                  temp _bits_le_len (Vlong (Int64.repr bits_le_len));
                  temp _bit_count (Vlong (Int64.repr bit_count));
                  temp _out out;
                  temp _out_cap (Vlong (Int64.repr out_cap));
                  temp _out_len out_len)
           SEP (data_at Tsh tulong
                  (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)))
                  v_byte_count;
                data_at sh_bits
                  (tarray tuchar (ssz_bits_to_bytes bit_count))
                  (map Vubyte bytes) bits_le)).
        -- solve_impossible_branch.
        -- forward.
           forward.
           entailer!.
           unfold bool2val, Int64.cmpu, Int64.ltu.
           rewrite !Int64.unsigned_repr by
             (unfold uint64_max, uint64_modulus, Int64.max_unsigned,
                Int64.modulus in *; simpl in *; lia).
           change (serialize_bitvector_bits_le_len
             (Serialize_bitvector_padding_invalid bits_le out out_len
                sh_bits bytes bits_le_len bit_count out_cap))
             with bits_le_len.
           replace
             (if zlt bits_le_len (ssz_bits_to_bytes bit_count)
              then true else false)
             with false by
             (destruct (zlt bits_le_len
                (ssz_bits_to_bytes bit_count)); [lia | reflexivity]).
           reflexivity.
        -- forward_if
            (PROP ()
             LOCAL (temp _err (Vint (Int.repr ssz_err_encoding_invalid));
                    temp _t'3 (Vint Int.zero);
                    temp _t'9
                      (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)));
                    temp _t'4 (Vint Int.one);
                    lvar _byte_count tulong v_byte_count;
                    temp _bits_le bits_le;
                    temp _bits_le_len (Vlong (Int64.repr bits_le_len));
                    temp _bit_count (Vlong (Int64.repr bit_count));
                    temp _out out;
                    temp _out_cap (Vlong (Int64.repr out_cap));
                    temp _out_len out_len)
             SEP (data_at Tsh tulong
                    (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)))
                    v_byte_count;
                  data_at sh_bits
                    (tarray tuchar (ssz_bits_to_bytes bit_count))
                    (map Vubyte bytes) bits_le)).
           ++ solve_impossible_branch.
           ++ forward_if
                (PROP ()
                 LOCAL (temp _err
                          (Vint (Int.repr ssz_err_encoding_invalid));
                        temp _t'3 (Vint Int.zero);
                        temp _t'9
                          (Vlong (Int64.repr
                            (ssz_bits_to_bytes bit_count)));
                        temp _t'4 (Vint Int.one);
                        lvar _byte_count tulong v_byte_count;
                        temp _bits_le bits_le;
                        temp _bits_le_len
                          (Vlong (Int64.repr bits_le_len));
                        temp _bit_count (Vlong (Int64.repr bit_count));
                        temp _out out;
                        temp _out_cap (Vlong (Int64.repr out_cap));
                        temp _out_len out_len)
                 SEP (data_at Tsh tulong
                        (Vlong (Int64.repr
                          (ssz_bits_to_bytes bit_count)))
                        v_byte_count;
                      data_at sh_bits
                        (tarray tuchar (ssz_bits_to_bytes bit_count))
                        (map Vubyte bytes) bits_le)).
              ** forward.
                 entailer!.
                 rewrite !Int64.Z_mod_modulus_eq.
                 rewrite (Z.mod_small bit_count Int64.modulus) by
                   (unfold uint64_max, uint64_modulus in *;
                    change Int64.modulus with 18446744073709551616;
                    simpl in *; lia).
                 rewrite (Z.mod_small (bit_count mod 8) Int64.modulus) by
                   (change Int64.modulus with 18446744073709551616;
                    lia).
                 lia.
                 change (Int.zero_ext 8
                   (Int.zero_ext 8
                     (Int.sub
                       (Int.shl (Int.repr 1)
                         (Int64.loword
                           (Int64.modu (Int64.repr bit_count)
                             (Int64.repr (Int.unsigned (Int.repr 8))))))
                       (Int.repr 1)))) with
                   (bitvector_c_mask bit_count).
                 forward.
                 forward.
                 entailer!.
                 apply is_int_u8_vubyte.
                 forward_if
                   (PROP ()
                    LOCAL (temp _err
                             (Vint (Int.repr ssz_err_encoding_invalid));
                           temp _t'3 (Vint Int.zero);
                           temp _t'9
                             (Vlong (Int64.repr
                               (ssz_bits_to_bytes bit_count)));
                           temp _t'4 (Vint Int.one);
                           lvar _byte_count tulong v_byte_count;
                           temp _bits_le bits_le;
                           temp _bits_le_len
                             (Vlong (Int64.repr bits_le_len));
                           temp _bit_count (Vlong (Int64.repr bit_count));
                           temp _out out;
                           temp _out_cap (Vlong (Int64.repr out_cap));
                           temp _out_len out_len)
                    SEP (data_at Tsh tulong
                           (Vlong (Int64.repr
                             (ssz_bits_to_bytes bit_count)))
                           v_byte_count;
                         data_at sh_bits
                           (tarray tuchar (ssz_bits_to_bytes bit_count))
                           (map Vubyte bytes) bits_le)).
                 --- forward.
                     entailer!.
                 --- exfalso.
                     eapply bitvector_padding_typed_false_contradiction
                       with (bit_count := bit_count) (bytes := bytes);
                       eauto; lia.
              ** exfalso.
                 apply Hmod_nonzero.
                 rewrite (int64_modu_repr_8 bit_count Hbit) in H2.
                 change Int64.zero with (Int64.repr 0) in H2.
                 eapply int64_repr_inj_uint64;
                   [pose proof (Z.mod_pos_bound bit_count 8 ltac:(lia));
                    unfold uint64_max, uint64_modulus; simpl; lia
                   | split;
                     [lia
                     | unfold uint64_max, uint64_modulus; simpl; lia]
                   | exact H2].
              ** forward_if
                   (PROP ()
                    LOCAL (temp _err
                             (Vint (Int.repr ssz_err_encoding_invalid));
                           temp _t'3 (Vint Int.zero);
                           temp _t'9
                             (Vlong (Int64.repr
                               (ssz_bits_to_bytes bit_count)));
                           temp _t'4 (Vint Int.one);
                           lvar _byte_count tulong v_byte_count;
                           temp _bits_le bits_le;
                           temp _bits_le_len
                             (Vlong (Int64.repr bits_le_len));
                           temp _bit_count (Vlong (Int64.repr bit_count));
                           temp _out out;
                           temp _out_cap (Vlong (Int64.repr out_cap));
                           temp _out_len out_len)
                    SEP (data_at Tsh tulong
                           (Vlong (Int64.repr
                             (ssz_bits_to_bytes bit_count)))
                           v_byte_count;
                         data_at sh_bits
                           (tarray tuchar (ssz_bits_to_bytes bit_count))
                           (map Vubyte bytes) bits_le)).
                 --- solve_impossible_branch.
                 --- forward.
                     entailer!.
           ++ entailer!.
      * entailer!.
    + forward.
      unfold serialize_bitvector_result,
        ssz_serialize_bitvector_result.
      simpl.
      replace (bit_count =? 0) with false by
        (symmetry; apply Z.eqb_neq; exact Hnz).
      rewrite Hok.
      replace
        (bits_le_len <? ssz_bits_to_bytes bit_count)
        with false by
        (symmetry; apply Z.ltb_ge; exact Hshort).
      rewrite Hpadding.
      entailer!.
  -
    change (serialize_bitvector_prop
      (Serialize_bitvector_valid bits_le sh_bits bytes bits_le_len
        bit_count out_cap out_case)) in H.
    destruct H as
      (Hlen & Hbit & Hnz & Hok & Hshort & Hpadding & Hbytes_len &
       Hsh_bits & Hcap & Hout_case).
    change (serialize_bitvector_pre
      (Serialize_bitvector_valid bits_le sh_bits bytes bits_le_len
        bit_count out_cap out_case))
      with ((data_at sh_bits
        (tarray tuchar (ssz_bits_to_bytes bit_count))
        (map Vubyte bytes) bits_le *
        serialize_output_pre out_case
          (ssz_bits_to_bytes bit_count))%logic).
    change (serialize_bitvector_post
      (Serialize_bitvector_valid bits_le sh_bits bytes bits_le_len
        bit_count out_cap out_case))
      with ((data_at sh_bits
        (tarray tuchar (ssz_bits_to_bytes bit_count))
        (map Vubyte bytes) bits_le *
        serialize_output_post out_case
          (ssz_bits_to_bytes bit_count)
          (ssz_serialize_bitvector_bytes bytes))%logic).
    Intros.
    pose proof (ssz_bits_to_bytes_range _ Hbit) as Hbyte_range.
    assert (Hbit_pos : 0 < bit_count) by lia.
    pose proof (ssz_bits_to_bytes_pos _ Hbit_pos) as Hbyte_pos.
    assert (Hlast_index :
      0 <= ssz_bits_to_bytes bit_count - 1 < Zlength bytes) by lia.
    pose proof (Z.mod_pos_bound bit_count 8 ltac:(lia)) as Hmod_bound.
    forward.
    forward.
    assert_PROP (isptr bits_le) as Hbits_ptr by entailer!.
    forward_if
      (PROP ()
       LOCAL (temp _err
                (Vint (Int.repr
                  (serialize_output_result out_case
                    (ssz_bits_to_bytes bit_count) out_cap)));
              lvar _byte_count tulong v_byte_count;
              temp _bits_le bits_le;
              temp _bits_le_len (Vlong (Int64.repr bits_le_len));
              temp _bit_count (Vlong (Int64.repr bit_count));
              temp _out (serialize_output_out out_case);
              temp _out_cap (Vlong (Int64.repr out_cap));
              temp _out_len (serialize_output_out_len out_case))
       SEP (data_at Tsh tulong
              (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)))
              v_byte_count;
            data_at sh_bits
              (tarray tuchar (ssz_bits_to_bytes bit_count))
              (map Vubyte bytes) bits_le;
            serialize_output_post out_case
              (ssz_bits_to_bytes bit_count)
              (ssz_serialize_bitvector_bytes bytes))).
    + solve_bit_count_zero bit_count Hbit Hnz.
    + forward_call (bit_count, v_byte_count, Tsh).
      rewrite Hok.
      forward_if
        (PROP ()
           LOCAL (temp _err
                    (Vint (Int.repr
                      (serialize_output_result out_case
                        (ssz_bits_to_bytes bit_count) out_cap)));
                  lvar _byte_count tulong v_byte_count;
                temp _bits_le bits_le;
                temp _bits_le_len (Vlong (Int64.repr bits_le_len));
                temp _bit_count (Vlong (Int64.repr bit_count));
                temp _out (serialize_output_out out_case);
                temp _out_cap (Vlong (Int64.repr out_cap));
                temp _out_len (serialize_output_out_len out_case))
         SEP (data_at Tsh tulong
                (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)))
                v_byte_count;
              data_at sh_bits
                (tarray tuchar (ssz_bits_to_bytes bit_count))
                (map Vubyte bytes) bits_le;
              serialize_output_post out_case
                (ssz_bits_to_bytes bit_count)
                (ssz_serialize_bitvector_bytes bytes))).
      * solve_impossible_branch.
      * forward_if
          (PROP ()
           LOCAL (temp _t'3 (Vint Int.zero);
                  temp _t'9
                    (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)));
                  temp _t'4 (Vint Int.one);
                  temp _err (Vint (Int.repr ssz_success));
                  lvar _byte_count tulong v_byte_count;
                  temp _bits_le bits_le;
                  temp _bits_le_len (Vlong (Int64.repr bits_le_len));
                  temp _bit_count (Vlong (Int64.repr bit_count));
                  temp _out (serialize_output_out out_case);
                  temp _out_cap (Vlong (Int64.repr out_cap));
                  temp _out_len (serialize_output_out_len out_case))
           SEP (data_at Tsh tulong
                  (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)))
                  v_byte_count;
                data_at sh_bits
                  (tarray tuchar (ssz_bits_to_bytes bit_count))
                  (map Vubyte bytes) bits_le;
                serialize_output_pre out_case
                  (ssz_bits_to_bytes bit_count))).
        -- solve_impossible_branch.
        -- forward.
           forward.
           entailer!.
           unfold bool2val, Int64.cmpu, Int64.ltu.
           rewrite !Int64.unsigned_repr by
             (unfold uint64_max, uint64_modulus, Int64.max_unsigned,
                Int64.modulus in *; simpl in *; lia).
           change (serialize_bitvector_bits_le_len
             (Serialize_bitvector_valid bits_le sh_bits bytes bits_le_len
                bit_count out_cap out_case))
             with bits_le_len.
           replace
             (if zlt bits_le_len (ssz_bits_to_bytes bit_count)
              then true else false)
             with false by
             (destruct (zlt bits_le_len
                (ssz_bits_to_bytes bit_count)); [lia | reflexivity]).
           reflexivity.
        -- forward_if
            (PROP ()
               LOCAL (temp _err
                        (Vint (Int.repr
                          (serialize_output_result out_case
                            (ssz_bits_to_bytes bit_count) out_cap)));
                      lvar _byte_count tulong v_byte_count;
                    temp _bits_le bits_le;
                    temp _bits_le_len (Vlong (Int64.repr bits_le_len));
                    temp _bit_count (Vlong (Int64.repr bit_count));
                    temp _out (serialize_output_out out_case);
                    temp _out_cap (Vlong (Int64.repr out_cap));
                    temp _out_len (serialize_output_out_len out_case))
             SEP (data_at Tsh tulong
                    (Vlong (Int64.repr (ssz_bits_to_bytes bit_count)))
                    v_byte_count;
                  data_at sh_bits
                    (tarray tuchar (ssz_bits_to_bytes bit_count))
                    (map Vubyte bytes) bits_le;
                  serialize_output_post out_case
                    (ssz_bits_to_bytes bit_count)
                    (ssz_serialize_bitvector_bytes bytes))).
           ++ solve_impossible_branch.
           ++ forward_if
                (PROP ()
                 LOCAL (temp _err (Vint (Int.repr ssz_success));
                        temp _t'3 (Vint Int.zero);
                        temp _t'9
                          (Vlong (Int64.repr
                            (ssz_bits_to_bytes bit_count)));
                        temp _t'4 (Vint Int.one);
                        lvar _byte_count tulong v_byte_count;
                        temp _bits_le bits_le;
                        temp _bits_le_len
                          (Vlong (Int64.repr bits_le_len));
                        temp _bit_count (Vlong (Int64.repr bit_count));
                        temp _out (serialize_output_out out_case);
                        temp _out_cap (Vlong (Int64.repr out_cap));
                        temp _out_len (serialize_output_out_len out_case))
                 SEP (data_at Tsh tulong
                        (Vlong (Int64.repr
                          (ssz_bits_to_bytes bit_count)))
                        v_byte_count;
                      data_at sh_bits
                        (tarray tuchar (ssz_bits_to_bytes bit_count))
                        (map Vubyte bytes) bits_le;
                      serialize_output_pre out_case
                        (ssz_bits_to_bytes bit_count))).
              ** assert (Hmod_nonzero : bit_count mod 8 <> 0).
                 {
                   intro Hmod_zero.
                   apply H2.
                   rewrite (int64_modu_repr_8 bit_count Hbit).
                   rewrite Hmod_zero.
                   reflexivity.
                 }
                 forward.
                 entailer!.
                 rewrite !Int64.Z_mod_modulus_eq.
                 rewrite (Z.mod_small bit_count Int64.modulus) by
                   (unfold uint64_max, uint64_modulus in *;
                    change Int64.modulus with 18446744073709551616;
                    simpl in *; lia).
                 rewrite
                   (Z.mod_small (bit_count mod 8) Int64.modulus) by
                   (change Int64.modulus with 18446744073709551616;
                    lia).
                 lia.
                 change (Int.zero_ext 8
                   (Int.zero_ext 8
                     (Int.sub
                       (Int.shl (Int.repr 1)
                         (Int64.loword
                           (Int64.modu (Int64.repr bit_count)
                             (Int64.repr
                               (Int.unsigned (Int.repr 8))))))
                       (Int.repr 1)))) with
                   (bitvector_c_mask bit_count).
                 forward.
                 forward.
                 entailer!.
                 apply is_int_u8_vubyte.
                 forward_if
                   (PROP ()
                    LOCAL (temp _err (Vint (Int.repr ssz_success));
                           temp _t'3 (Vint Int.zero);
                           temp _t'9
                             (Vlong (Int64.repr
                               (ssz_bits_to_bytes bit_count)));
                           temp _t'4 (Vint Int.one);
                           lvar _byte_count tulong v_byte_count;
                           temp _bits_le bits_le;
                           temp _bits_le_len
                             (Vlong (Int64.repr bits_le_len));
                           temp _bit_count (Vlong (Int64.repr bit_count));
                           temp _out (serialize_output_out out_case);
                           temp _out_cap (Vlong (Int64.repr out_cap));
                           temp _out_len
                             (serialize_output_out_len out_case))
                    SEP (data_at Tsh tulong
                           (Vlong (Int64.repr
                             (ssz_bits_to_bytes bit_count)))
                           v_byte_count;
                         data_at sh_bits
                           (tarray tuchar
                             (ssz_bits_to_bytes bit_count))
                           (map Vubyte bytes) bits_le;
                         serialize_output_pre out_case
                           (ssz_bits_to_bytes bit_count))).
                 --- exfalso.
                     eapply bitvector_padding_typed_true_contradiction
                       with (bit_count := bit_count) (bytes := bytes);
                       eauto; lia.
                 --- forward.
                     entailer!.
              ** forward.
                 entailer!.
              ** forward_if
                   (PROP ()
                      LOCAL (temp _err
                               (Vint (Int.repr
                                 (serialize_output_result out_case
                                   (ssz_bits_to_bytes bit_count) out_cap)));
                             lvar _byte_count tulong v_byte_count;
                             temp _bits_le bits_le;
                           temp _bits_le_len
                             (Vlong (Int64.repr bits_le_len));
                           temp _bit_count (Vlong (Int64.repr bit_count));
                           temp _out (serialize_output_out out_case);
                           temp _out_cap (Vlong (Int64.repr out_cap));
                           temp _out_len
                             (serialize_output_out_len out_case))
                    SEP (data_at Tsh tulong
                           (Vlong (Int64.repr
                             (ssz_bits_to_bytes bit_count)))
                           v_byte_count;
                         data_at sh_bits
                           (tarray tuchar (ssz_bits_to_bytes bit_count))
                           (map Vubyte bytes) bits_le;
                         serialize_output_post out_case
                           (ssz_bits_to_bytes bit_count)
                           (ssz_serialize_bitvector_bytes bytes))).
                 --- destruct out_case as
                       [out0 | out_len0 sh_len
                       | out0 out_len0 sh_len
                       | out0 out_len0 sh_len sh_out];
                     simpl in Hout_case;
                     forward.
                     forward_call
                         (Prepare_output_len_null out0,
                          ssz_bits_to_bytes bit_count, out_cap).
                     entailer!.
                     forward.
                     forward_if
                       (PROP ()
                        LOCAL (temp _t'2 (Vint Int.zero);
                               temp _err
                                 (Vint
                                   (Int.repr ssz_err_invalid_argument));
                               temp _t'1
                                 (Vint
                                   (Int.repr ssz_err_invalid_argument));
                                 temp _t'6
                                   (Vlong (Int64.repr
                                     (ssz_bits_to_bytes bit_count)));
                                 lvar _byte_count tulong v_byte_count;
                                 temp _bits_le bits_le;
                               temp _bits_le_len
                                 (Vlong (Int64.repr bits_le_len));
                               temp _bit_count
                                 (Vlong (Int64.repr bit_count));
                               temp _out out0;
                               temp _out_cap (Vlong (Int64.repr out_cap));
                               temp _out_len nullval)
                        SEP (data_at Tsh tulong
                               (Vlong (Int64.repr
                                 (ssz_bits_to_bytes bit_count)))
                               v_byte_count;
                             data_at sh_bits
                               (tarray tuchar
                                 (ssz_bits_to_bytes bit_count))
                               (map Vubyte bytes) bits_le)).
                     ++++ solve_impossible_branch.
                     ++++ forward.
                       entailer!.
                     ++++ forward_if
                         (PROP ()
                          LOCAL (temp _err
                                   (Vint
                                     (Int.repr ssz_err_invalid_argument));
                                 temp _t'2 (Vint Int.zero);
                                 temp _t'1
                                   (Vint
                                     (Int.repr ssz_err_invalid_argument));
                                   temp _t'6
                                     (Vlong (Int64.repr
                                       (ssz_bits_to_bytes bit_count)));
                                   lvar _byte_count tulong v_byte_count;
                                   temp _bits_le bits_le;
                                 temp _bits_le_len
                                   (Vlong (Int64.repr bits_le_len));
                                 temp _bit_count
                                   (Vlong (Int64.repr bit_count));
                                 temp _out out0;
                                 temp _out_cap
                                   (Vlong (Int64.repr out_cap));
                                 temp _out_len nullval)
                          SEP (data_at Tsh tulong
                                 (Vlong (Int64.repr
                                   (ssz_bits_to_bytes bit_count)))
                                 v_byte_count;
                               data_at sh_bits
                                 (tarray tuchar
                                   (ssz_bits_to_bytes bit_count))
                                 (map Vubyte bytes) bits_le)).
                       ----- exfalso.
                         clear POSTCONDITION.
                         match goal with
                         | Hbad : 0 <> 0 |- _ =>
                             apply Hbad; reflexivity
                         end.
                       ----- forward.
                          entailer!.
                       ----- entailer!.
                     ++++ change
                       (serialize_output_pre
                         (Serialize_output_measure out_len0 sh_len)
                         (ssz_bits_to_bytes bit_count))
                       with (data_at_ sh_len tulong out_len0).
                       forward_call
                         (Prepare_output_measure_only out_len0 sh_len,
                          ssz_bits_to_bytes bit_count, out_cap).
                       forward.
                       forward_if
                         (PROP ()
                          LOCAL (temp _t'2 (Vint Int.zero);
                                 temp _err (Vint (Int.repr ssz_success));
                                 temp _t'1 (Vint (Int.repr ssz_success));
                                   temp _t'6
                                     (Vlong (Int64.repr
                                       (ssz_bits_to_bytes bit_count)));
                                   lvar _byte_count tulong v_byte_count;
                                   temp _bits_le bits_le;
                                 temp _bits_le_len
                                   (Vlong (Int64.repr bits_le_len));
                                 temp _bit_count
                                   (Vlong (Int64.repr bit_count));
                                 temp _out nullval;
                                 temp _out_cap
                                   (Vlong (Int64.repr out_cap));
                                 temp _out_len out_len0)
                          SEP (data_at Tsh tulong
                                 (Vlong (Int64.repr
                                   (ssz_bits_to_bytes bit_count)))
                                 v_byte_count;
                               data_at sh_bits
                                 (tarray tuchar
                                   (ssz_bits_to_bytes bit_count))
                                 (map Vubyte bytes) bits_le;
                               data_at sh_len tulong
                                 (Vlong (Int64.repr
                                   (ssz_bits_to_bytes bit_count)))
                                 out_len0)).
                       ----- forward.
                         entailer!.
                         simpl.
                         entailer!.
                       ----- exfalso.
                         clear POSTCONDITION.
                         match goal with
                         | Hbad : Int.repr
                             (prepare_output_result
                               (Prepare_output_measure_only out_len0 sh_len)
                               (ssz_bits_to_bytes bit_count) out_cap) <>
                             Int.repr 0 |- _ =>
                             unfold prepare_output_result,
                               ssz_internal_prepare_output_result,
                               prepare_output_has_out_len,
                               prepare_output_has_out, ssz_success in Hbad;
                             simpl in Hbad;
                             apply Hbad; reflexivity
                         end.
                       ----- forward_if
                           (PROP ()
                            LOCAL (temp _err
                                     (Vint (Int.repr ssz_success));
                                   temp _t'2 (Vint Int.zero);
                                   temp _t'1
                                     (Vint (Int.repr ssz_success));
                                     temp _t'6
                                       (Vlong (Int64.repr
                                         (ssz_bits_to_bytes bit_count)));
                                     lvar _byte_count tulong v_byte_count;
                                     temp _bits_le bits_le;
                                   temp _bits_le_len
                                     (Vlong (Int64.repr bits_le_len));
                                   temp _bit_count
                                     (Vlong (Int64.repr bit_count));
                                   temp _out nullval;
                                   temp _out_cap
                                     (Vlong (Int64.repr out_cap));
                                   temp _out_len out_len0)
                            SEP (data_at Tsh tulong
                                   (Vlong (Int64.repr
                                     (ssz_bits_to_bytes bit_count)))
                                   v_byte_count;
                                 data_at sh_bits
                                   (tarray tuchar
                                     (ssz_bits_to_bytes bit_count))
                                   (map Vubyte bytes) bits_le;
                                 data_at sh_len tulong
                                   (Vlong (Int64.repr
                                     (ssz_bits_to_bytes bit_count)))
                                   out_len0)).
                         +++++ solve_impossible_branch.
                         +++++ forward.
                            entailer!.
                         +++++ entailer!.
                     ++++ destruct Hout_case as
                       (Hout_ptr & Hsh_len & Hsmall).
                       change
                         (serialize_output_pre
                           (Serialize_output_buffer_small out0 out_len0
                             sh_len)
                           (ssz_bits_to_bytes bit_count))
                         with ((data_at_ sh_len tulong out_len0 *
                                valid_pointer out0)%logic).
                       Intros.
                       forward_call
                         (Prepare_output_with_out out0 out_len0 sh_len,
                          ssz_bits_to_bytes bit_count, out_cap).
                       change (prepare_output_pre
                         (Prepare_output_with_out out0 out_len0 sh_len))
                         with ((data_at_ sh_len tulong out_len0 *
                                valid_pointer out0)%logic).
                       cancel.
                       split; auto.
                       forward.
                       forward_if
                         (PROP ()
                          LOCAL (temp _t'2 (Vint Int.zero);
                                 temp _err
                                   (Vint
                                     (Int.repr ssz_err_buffer_too_small));
                                 temp _t'1
                                   (Vint
                                     (Int.repr ssz_err_buffer_too_small));
                                   temp _t'6
                                     (Vlong (Int64.repr
                                       (ssz_bits_to_bytes bit_count)));
                                   lvar _byte_count tulong v_byte_count;
                                   temp _bits_le bits_le;
                                 temp _bits_le_len
                                   (Vlong (Int64.repr bits_le_len));
                                 temp _bit_count
                                   (Vlong (Int64.repr bit_count));
                                 temp _out out0;
                                 temp _out_cap
                                   (Vlong (Int64.repr out_cap));
                                 temp _out_len out_len0)
                          SEP (data_at Tsh tulong
                                 (Vlong (Int64.repr
                                   (ssz_bits_to_bytes bit_count)))
                                 v_byte_count;
                               data_at sh_bits
                                 (tarray tuchar
                                   (ssz_bits_to_bytes bit_count))
                                 (map Vubyte bytes) bits_le;
                               data_at sh_len tulong
                                 (Vlong (Int64.repr
                                   (ssz_bits_to_bytes bit_count)))
                                 out_len0;
                               valid_pointer out0)).
                       ----- exfalso.
                         unfold prepare_output_result,
                           ssz_internal_prepare_output_result,
                           prepare_output_has_out_len,
                           prepare_output_has_out,
                           ssz_err_buffer_too_small in H3.
                         simpl in H3.
                         replace
                           (out_cap <? ssz_bits_to_bytes bit_count)
                           with true in H3 by
                           (symmetry; apply Z.ltb_lt; exact Hsmall).
                         discriminate H3.
                       ----- forward.
                         entailer!.
                         unfold prepare_output_result,
                           ssz_internal_prepare_output_result,
                           prepare_output_has_out_len,
                           prepare_output_has_out,
                           ssz_err_buffer_too_small.
                         simpl.
                         replace
                           (out_cap <? ssz_bits_to_bytes bit_count)
                           with true by
                           (symmetry; apply Z.ltb_lt; exact Hsmall).
                         split; reflexivity.
                         simpl.
                         cancel.
                       ----- forward_if
                           (PROP ()
                            LOCAL (temp _err
                                     (Vint
                                       (Int.repr
                                         ssz_err_buffer_too_small));
                                   temp _t'2 (Vint Int.zero);
                                   temp _t'1
                                     (Vint
                                       (Int.repr
                                         ssz_err_buffer_too_small));
                                     temp _t'6
                                       (Vlong (Int64.repr
                                         (ssz_bits_to_bytes bit_count)));
                                     lvar _byte_count tulong v_byte_count;
                                     temp _bits_le bits_le;
                                   temp _bits_le_len
                                     (Vlong (Int64.repr bits_le_len));
                                   temp _bit_count
                                     (Vlong (Int64.repr bit_count));
                                   temp _out out0;
                                   temp _out_cap
                                     (Vlong (Int64.repr out_cap));
                                   temp _out_len out_len0)
                            SEP (data_at Tsh tulong
                                   (Vlong (Int64.repr
                                     (ssz_bits_to_bytes bit_count)))
                                   v_byte_count;
                                 data_at sh_bits
                                   (tarray tuchar
                                     (ssz_bits_to_bytes bit_count))
                                   (map Vubyte bytes) bits_le;
                                 data_at sh_len tulong
                                   (Vlong (Int64.repr
                                     (ssz_bits_to_bytes bit_count)))
                                   out_len0;
                                 valid_pointer out0)).
                         +++++ solve_impossible_branch.
                         +++++ forward.
                            entailer!.
                         +++++ unfold serialize_output_result,
                              ssz_internal_prepare_output_result,
                              serialize_output_has_out_len,
                              serialize_output_has_out,
                              ssz_err_buffer_too_small.
                            replace
                              (out_cap <?
                                ssz_bits_to_bytes bit_count)
                              with true by
                              (symmetry; apply Z.ltb_lt; exact Hsmall).
                            entailer!.
                            simpl.
                            unfold ssz_serialize_bitvector_bytes.
                            cancel.
                     ++++ destruct Hout_case as
                       (Hout_ptr & Hsh_len & Hsh_out & Hcap_ok).
                       simpl.
                       Intros.
                       forward_call
                         (Prepare_output_with_out out0 out_len0 sh_len,
                          ssz_bits_to_bytes bit_count, out_cap).
                       change (prepare_output_pre
                         (Prepare_output_with_out out0 out_len0 sh_len))
                         with ((data_at_ sh_len tulong out_len0 *
                                valid_pointer out0)%logic).
                       entailer!.
                       split; auto.
                       forward.
                       forward_if
                         (PROP ()
                          LOCAL (temp _t'2 (Vint Int.one);
                                 temp _err (Vint (Int.repr ssz_success));
                                 temp _t'1 (Vint (Int.repr ssz_success));
                                 temp _t'6
                                   (Vlong (Int64.repr
                                     (ssz_bits_to_bytes bit_count)));
                                 lvar _byte_count tulong v_byte_count;
                                 temp _bits_le bits_le;
                                 temp _bits_le_len
                                   (Vlong (Int64.repr bits_le_len));
                                 temp _bit_count
                                   (Vlong (Int64.repr bit_count));
                                 temp _out out0;
                                 temp _out_cap
                                   (Vlong (Int64.repr out_cap));
                                 temp _out_len out_len0)
                          SEP (data_at Tsh tulong
                                 (Vlong (Int64.repr
                                   (ssz_bits_to_bytes bit_count)))
                                 v_byte_count;
                               data_at sh_bits
                                 (tarray tuchar
                                   (ssz_bits_to_bytes bit_count))
                                 (map Vubyte bytes) bits_le;
                               data_at sh_len tulong
                                 (Vlong (Int64.repr
                                   (ssz_bits_to_bytes bit_count)))
                                 out_len0;
                               data_at_ sh_out
                                 (tarray tuchar
                                   (ssz_bits_to_bytes bit_count))
                                 out0;
                               valid_pointer out0)).
                       ----- forward.
                         entailer!.
                         simpl.
                         unfold prepare_output_result,
                           ssz_internal_prepare_output_result,
                           prepare_output_has_out_len,
                           prepare_output_has_out, ssz_success.
                         simpl.
                         replace
                           (out_cap <? ssz_bits_to_bytes bit_count)
                           with false by
                           (symmetry; apply Z.ltb_ge; exact Hcap_ok).
                         symmetry.
                         destruct out0; inversion Hout_ptr; cbn;
                           reflexivity.
                         simpl.
                         cancel.
                       ----- exfalso.
                         match goal with
                         | Hbad : Int.repr
                             (prepare_output_result
                               (Prepare_output_with_out out0 out_len0 sh_len)
                               (ssz_bits_to_bytes bit_count) out_cap) <>
                             Int.repr 0 |- _ =>
                             unfold prepare_output_result,
                               ssz_internal_prepare_output_result,
                               prepare_output_has_out_len,
                               prepare_output_has_out, ssz_success in Hbad;
                             simpl in Hbad;
                             replace
                               (out_cap <?
                                 ssz_bits_to_bytes bit_count)
                               with false in Hbad by
                               (symmetry; apply Z.ltb_ge; exact Hcap_ok);
                             apply Hbad; reflexivity
                         end.
                       ----- forward_if
                           (PROP ()
                            LOCAL (temp _err
                                     (Vint (Int.repr ssz_success));
                                   temp _t'2 (Vint Int.one);
                                   temp _t'1
                                     (Vint (Int.repr ssz_success));
                                   temp _t'6
                                     (Vlong (Int64.repr
                                       (ssz_bits_to_bytes bit_count)));
                                   lvar _byte_count tulong v_byte_count;
                                   temp _bits_le bits_le;
                                   temp _bits_le_len
                                     (Vlong (Int64.repr bits_le_len));
                                   temp _bit_count
                                     (Vlong (Int64.repr bit_count));
                                   temp _out out0;
                                   temp _out_cap
                                     (Vlong (Int64.repr out_cap));
                                   temp _out_len out_len0)
                            SEP (data_at Tsh tulong
                                   (Vlong (Int64.repr
                                     (ssz_bits_to_bytes bit_count)))
                                   v_byte_count;
                                 data_at sh_bits
                                   (tarray tuchar
                                     (ssz_bits_to_bytes bit_count))
                                   (map Vubyte bytes) bits_le;
                                 data_at sh_len tulong
                                   (Vlong (Int64.repr
                                     (ssz_bits_to_bytes bit_count)))
                                   out_len0;
                                 data_at sh_out
                                   (tarray tuchar
                                     (ssz_bits_to_bytes bit_count))
                                   (map Vubyte
                                     (ssz_serialize_bitvector_bytes bytes))
                                   out0;
                                 valid_pointer out0)).
                         +++++ forward.
                              forward_call
                                (ssz_bits_to_bytes bit_count,
                                 map Vubyte bytes, out0, bits_le,
                                 sh_out, sh_bits).
                              {
                                repeat split;
                                  try assumption;
                                  try (rewrite Zlength_map;
                                       exact Hbytes_len);
                                  try lia.
                              }
                              unfold ssz_serialize_bitvector_bytes.
                              entailer!; cancel.
                           +++++ solve_impossible_branch.
                           +++++ entailer!.
                              unfold serialize_output_result,
                                ssz_internal_prepare_output_result,
                                serialize_output_has_out_len,
                                serialize_output_has_out, ssz_success.
                              replace
                                (out_cap <?
                                  ssz_bits_to_bytes bit_count)
                                with false by
                                (symmetry; apply Z.ltb_ge; exact Hcap_ok).
                              reflexivity.
                              simpl.
                              cancel.
                   --- exfalso.
                       clear POSTCONDITION.
                       unfold ssz_success in *.
                       simpl in *.
                       congruence.
                   + forward.
        unfold serialize_bitvector_result,
          ssz_serialize_bitvector_result.
        simpl.
        replace (bit_count =? 0) with false by
          (symmetry; apply Z.eqb_neq; exact Hnz).
        rewrite Hok.
        replace
          (bits_le_len <? ssz_bits_to_bytes bit_count)
          with false by
          (symmetry; apply Z.ltb_ge; exact Hshort).
        rewrite Hpadding.
        unfold serialize_output_result.
        entailer!.
  Qed.
