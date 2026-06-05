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

Ltac solve_impossible_branch :=
  exfalso;
  match goal with
  | Hnull : ?p = nullval, Hptr : isptr ?p |- _ =>
      subst p; inversion Hptr
  | Hnull : nullval = ?p, Hptr : isptr ?p |- _ =>
      subst p; inversion Hptr
  | Hbad : Int64.repr 0 <> Int64.repr 0 |- _ =>
      apply Hbad; reflexivity
  | Hbad : Int.one = Int.zero |- _ =>
      discriminate Hbad
  | Hbad : Int.zero = Int.one |- _ =>
      discriminate Hbad
  | Hbad : Int.one <> Int.zero |- _ =>
      apply Hbad; discriminate
  | Hbad : Int.zero <> Int.zero |- _ =>
      apply Hbad; reflexivity
  | Hbad : 0 <> 0 |- _ =>
      apply Hbad; reflexivity
  | Hbad : false = true |- _ =>
      discriminate Hbad
  | Hbad : true = false |- _ =>
      discriminate Hbad
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
