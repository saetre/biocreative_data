/*
 *      Linear-chain CRF training.
 *
 * Copyright (c) 2007-2010, Naoaki Okazaki
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the names of the authors nor the names of its contributors
 *       may be used to endorse or promote products derived from this
 *       software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* $Id: crf1m_learn.c 176 2010-07-14 09:31:04Z naoaki $ */

#ifdef    HAVE_CONFIG_H
#include <config.h>
#endif/*HAVE_CONFIG_H*/

#include <os.h>

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#include <crfsuite.h>
#include "params.h"
#include "mt19937ar.h"

#include "logging.h"
#include "crf1m.h"

#define    FEATURE(trainer, k) \
    (&(trainer)->features[(k)])
#define    ATTRIBUTE(trainer, a) \
    (&(trainer)->attributes[(a)])
#define    TRANSITION_FROM(trainer, i) \
    (&(trainer)->forward_trans[(i)])
#define    TRANSITION_TO(trainer, j) \
    (&(trainer)->backward_trans[(j)])
#define    TRANSITION_BOS(trainer) \
    (&(trainer)->bos_trans)
#define    TRANSITION_EOS(trainer) \
    (&(trainer)->eos_trans)

void crf1ml_set_labels(crf1ml_t* trainer, const crf_sequence_t* seq)
{
    int t;
    crf1m_context_t* ctx = trainer->ctx;
    const crf_item_t* item = NULL;
    const int T = seq->num_items;

    ctx->num_items = T;
    for (t = 0;t < T;++t) {
        item = &seq->items[t];
        ctx->labels[t] = item->label;
    }
}

void
crf1ml_state_score(
    crf1ml_t* trainer,
    const crf_sequence_t* seq,
    const floatval_t* w,
    const int K,
    floatval_t dummy
    )
{
    int a, i, l, t, r, fid;
    floatval_t scale, *state = NULL;
    crf1m_context_t* ctx = trainer->ctx;
    const crf_item_t* item = NULL;
    const crf1ml_feature_t* f = NULL;
    const feature_refs_t* attr = NULL;
    const int T = seq->num_items;
    const int L = trainer->num_labels;

    /* Loop over the items in the sequence. */
    for (t = 0;t < T;++t) {
        item = &seq->items[t];
        state = STATE_SCORE_AT(ctx, t);

        /* Initialize the state scores at position #t as zero. */
        for (i = 0;i < L;++i) {
            state[i] = 0;
        }

        /* Loop over the contents (attributes) attached to the item. */
        for (i = 0;i < item->num_contents;++i) {
            /* Access the list of state features associated with the attribute. */
            a = item->contents[i].aid;
            attr = ATTRIBUTE(trainer, a);
            /* A scale usually represents the atrribute frequency in the item. */
            scale = item->contents[i].scale * dummy;

            /* Loop over the state features associated with the attribute. */
            for (r = 0;r < attr->num_features;++r) {
                /* The state feature #(attr->fids[r]), which is represented by
                   the attribute #a, outputs the label #(f->dst). */
                fid = attr->fids[r];
                f = FEATURE(trainer, fid);
                l = f->dst;
                state[l] += w[fid] * scale;
            }
        }
    }
}

void crf1ml_transition_score(
    crf1ml_t* trainer,
    const floatval_t* w,
    const int K,
    floatval_t dummy
    )
{
    int i, j, r, fid;
    floatval_t *trans = NULL;
    crf1m_context_t* ctx = trainer->ctx;
    const crf1ml_feature_t* f = NULL;
    const feature_refs_t* edge = NULL;
    const int L = trainer->num_labels;

    /* Initialize all transition scores as zero. */
    for (i = 0;i <= L;++i) {
        trans = TRANS_SCORE_FROM(ctx, i);
        for (j = 0;j <= L;++j) {
            trans[j] = 0. * dummy;
        }
    }

    /* Compute transition scores from BOS to labels. */
    trans = TRANS_SCORE_FROM(ctx, L);
    edge = TRANSITION_BOS(trainer);
    for (r = 0;r < edge->num_features;++r) {
        /* Transition feature from BOS to #(f->dst). */
        fid = edge->fids[r];
        f = FEATURE(trainer, fid);
        trans[f->dst] = w[fid] * dummy;
    }

    /* Compute transition scores between two labels. */
    for (i = 0;i < L;++i) {
        trans = TRANS_SCORE_FROM(ctx, i);
        edge = TRANSITION_FROM(trainer, i);
        for (r = 0;r < edge->num_features;++r) {
            /* Transition feature from #i to #(f->dst). */
            fid = edge->fids[r];
            f = FEATURE(trainer, fid);
            trans[f->dst] = w[fid] * dummy;
        }        
    }

    /* Compute transition scores from labels to EOS. */
    edge = TRANSITION_EOS(trainer);
    for (r = 0;r < edge->num_features;++r) {
        /* Transition feature from #(f->src) to EOS. */
        fid = edge->fids[r];
        f = FEATURE(trainer, fid);
        trans = TRANS_SCORE_FROM(ctx, f->src);
        trans[L] = w[fid] * dummy;
    }    
}

#define OEXP    1

void crf1ml_enum_features(crf1ml_t* trainer, const crf_sequence_t* seq, update_feature_t func)
{
    int a, c, i, j, t, r, fid;
    floatval_t coeff, scale, *prob = trainer->prob;
    crf1m_context_t* ctx = trainer->ctx;
    const floatval_t lognorm = ctx->log_norm;
    const floatval_t *fwd = NULL, *bwd = NULL, *state = NULL, *edge = NULL;
    crf1ml_feature_t* f = NULL;
    const feature_refs_t *attr = NULL, *trans = NULL;
    const crf_item_t* item = NULL;
    const int T = seq->num_items;
    const int L = trainer->num_labels;

    /*
        This code prioritizes the computation speed over the simplicity:
        it reduces the number of repeated calculations at the expense of
        its beauty. This routine calculates model expectations of features
        in four steps:

        (i)   state features at position #0 and BOS transition features
        (ii)  state features at position #T-1 and EOS transition features
        (iii) state features at position #t (0 < t < T-1)
        (iv)  transition features.

        Notice that the partition factor (norm) is computed as:
            norm = 1 / (C[0] * ... * C[T]).
     */

    /*
        (i) Calculate the probabilities at (0, i):
            p(0,i) = fwd[0][i] * bwd[0][i] / norm
                   = (fwd'[0][i] / C[0]) * (bwd'[0][i] / (C[0] * ... C[T-1])) * (C[0] * ... * C[T])
                   = (C[T] / C[0]) * fwd'[0][i] * bwd'[0][i]
        to compute expectations of transition features from BOS
        and state features at position #0.
     */
    fwd = FORWARD_SCORE_AT(ctx, 0);
    bwd = BACKWARD_SCORE_AT(ctx, 0);
    coeff = ctx->scale_factor[T] / ctx->scale_factor[0];
    for (i = 0;i < L;++i) {
        prob[i] = fwd[i] * bwd[i] * coeff;
    }

    /* Compute expectations for transition features from BOS. */
    trans = TRANSITION_BOS(trainer);
    for (r = 0;r < trans->num_features;++r) {
        fid = trans->fids[r];
        f = FEATURE(trainer, fid);
        func(f, fid, prob[f->dst], 1., trainer, seq, 0);
    }

    /* Compute expectations for state features at position #0. */
    item = &seq->items[0];
    for (c = 0;c < item->num_contents;++c) {
        /* Access the attribute. */
        scale = item->contents[c].scale;
        a = item->contents[c].aid;
        attr = ATTRIBUTE(trainer, a);

        /* Loop over state features for the attribute. */
        for (r = 0;r < attr->num_features;++r) {
            /* Reuse the probability prob[f->dst]. */
            fid = attr->fids[r];
            f = FEATURE(trainer, fid);
            func(f, fid, prob[f->dst], scale, trainer, seq, 0);
        }
    }

    /*
        (ii) Calculate the probabilities at (T-1, i):
            p(T-1,i) = fwd[T-1][i] bwd[T-1][i] / norm
                     = (C[T] / C[T-1]) * fwd'[T-1][i] * bwd'[T-1][i]
        to compute expectations of transition features to EOS
        and state features at position #(T-1).
     */
    fwd = FORWARD_SCORE_AT(ctx, T-1);
    bwd = BACKWARD_SCORE_AT(ctx, T-1);
    coeff = ctx->scale_factor[T] / ctx->scale_factor[T-1];
    for (i = 0;i < L;++i) {
        prob[i] = fwd[i] * bwd[i] * coeff;
    }

    /* Compute expectations for transition features to EOS. */
    trans = TRANSITION_EOS(trainer);
    for (r = 0;r < trans->num_features;++r) {
        fid = trans->fids[r];
        f = FEATURE(trainer, fid);
        func(f, fid, prob[f->src], 1., trainer, seq, T-1);
    }

    /* Compute expectations for state features at position #(T-1). */
    item = &seq->items[T-1];
    for (c = 0;c < item->num_contents;++c) {
        /* Access the attribute. */
        scale = item->contents[c].scale;
        a = item->contents[c].aid;
        attr = ATTRIBUTE(trainer, a);

        /* Loop over state features for the attribute. */
        for (r = 0;r < attr->num_features;++r) {
            /* Reuse the probability prob[f->dst]. */
            fid = attr->fids[r];
            f = FEATURE(trainer, fid);
            func(f, fid, prob[f->dst], scale, trainer, seq, T-1);
        }
    }

    /*
        (iii) For 0 < t < T-1, calculate the probabilities at (t, i):
            p(t,i) = fwd[t][i] * bwd[t][i] / norm
                   = (C[T] / C[t]) * fwd'[t][i] * bwd'[t][i]
        to compute expectations of state features at position #t.
     */
    for (t = 1;t < T-1;++t) {
        fwd = FORWARD_SCORE_AT(ctx, t);
        bwd = BACKWARD_SCORE_AT(ctx, t);
        coeff = ctx->scale_factor[T] / ctx->scale_factor[t];

        /* Initialize the probabilities as -1, which means 'unfilled'.  */
        for (i = 0;i < L;++i) prob[i] = -1;

        /* Compute expectations for state features at position #t. */
        item = &seq->items[t];
        for (c = 0;c < item->num_contents;++c) {
            /* Access the attribute. */
            scale = item->contents[c].scale;
            a = item->contents[c].aid;
            attr = ATTRIBUTE(trainer, a);

            /* Loop over state features for the attribute. */
            for (r = 0;r < attr->num_features;++r) {
                fid = attr->fids[r];
                f = FEATURE(trainer, fid);
                i = f->dst;

                /* Reuse the probability prob[i] if it has been calculated. */
                if (prob[i] == -1) {
                    /* Unfilled: calculate the probability. */
                    prob[i] = fwd[i] * bwd[i] * coeff;
                }
                func(f, fid, prob[i], scale, trainer, seq, t);
            }
        }
    }

    /*
        (iv) Calculate the probabilities of the path (t, i) -> (t+1, j)
            p(t+1,j|t,i)
                = fwd[t][i] * edge[i][j] * state[t+1][j] * bwd[t+1][j] / norm
                = (fwd'[t][i] / (C[0] ... C[t])) * edge[i][j] * state[t+1][j] * (bwd'[t+1][j] / (C[t+1] ... C[T-1])) * (C[0] * ... * C[T])
                = fwd'[t][i] * edge[i][j] * state[t+1][j] * bwd'[t+1][j] * C[T]
        to compute expectations of transition features.
     */
    for (t = 0;t < T-1;++t) {
        fwd = FORWARD_SCORE_AT(ctx, t);
        state = STATE_SCORE_AT(ctx, t+1);
        bwd = BACKWARD_SCORE_AT(ctx, t+1);
        coeff = ctx->scale_factor[T];

        /* Loop over the labels (t, i) */
        for (i = 0;i < L;++i) {
            edge = TRANS_SCORE_FROM(ctx, i);
            trans = TRANSITION_FROM(trainer, i);
            for (r = 0;r < trans->num_features;++r) {
                fid = trans->fids[r];
                f = FEATURE(trainer, fid);
                j = f->dst;
                func(f, fid, fwd[i] * edge[j] * state[j] * bwd[j] * coeff, 1., trainer, seq, t);
            }
        }
    }
}

static int init_feature_references(crf1ml_t* trainer, const int A, const int L)
{
    int i, k;
    feature_refs_t *fl = NULL;
    const int K = trainer->num_features;
    const crf1ml_feature_t* features = trainer->features;

    /*
        The purpose of this routine is to collect references (indices) of:
        - state features fired by each attribute (trainer->attributes)
        - transition features pointing from each label (trainer->forward_trans)
        - transition features pointing to each label (trainer->backward_trans)
        - BOS features (trainer->bos_trans)
        - EOS features (trainer->eos_trans).
    */

    /* Initialize. */
    trainer->attributes = NULL;
    trainer->forward_trans = NULL;
    trainer->backward_trans = NULL;

    /* Allocate arrays for feature references. */
    trainer->attributes = (feature_refs_t*)calloc(A, sizeof(feature_refs_t));
    if (trainer->attributes == NULL) goto error_exit;
    trainer->forward_trans = (feature_refs_t*)calloc(L, sizeof(feature_refs_t));
    if (trainer->forward_trans == NULL) goto error_exit;
    trainer->backward_trans = (feature_refs_t*)calloc(L, sizeof(feature_refs_t));
    if (trainer->backward_trans == NULL) goto error_exit;
    memset(&trainer->bos_trans, 0, sizeof(feature_refs_t));
    memset(&trainer->eos_trans, 0, sizeof(feature_refs_t));

    /*
        Firstly, loop over the features to count the number of references.
        We don't want to use realloc() to avoid memory fragmentation.
     */
    for (k = 0;k < K;++k) {
        const crf1ml_feature_t *f = &features[k];
        switch (f->type) {
        case FT_STATE:
            trainer->attributes[f->src].num_features++;
            break;
        case FT_TRANS:
            trainer->forward_trans[f->src].num_features++;
            trainer->backward_trans[f->dst].num_features++;
            break;
        case FT_TRANS_BOS:
            trainer->bos_trans.num_features++;
            break;
        case FT_TRANS_EOS:
            trainer->eos_trans.num_features++;
            break;
        }
    }

    /*
        Secondarily, allocate memory blocks to store the feature references.
        We also clear fl->num_features fields, which will be used to indicate
        the offset positions in the last phase.
     */
    for (i = 0;i < trainer->num_attributes;++i) {
        fl = &trainer->attributes[i];
        fl->fids = (int*)calloc(fl->num_features, sizeof(int));
        if (fl->fids == NULL) goto error_exit;
        fl->num_features = 0;
    }
    for (i = 0;i < trainer->num_labels;++i) {
        fl = &trainer->forward_trans[i];
        fl->fids = (int*)calloc(fl->num_features, sizeof(int));
        if (fl->fids == NULL) goto error_exit;
        fl->num_features = 0;
        fl = &trainer->backward_trans[i];
        fl->fids = (int*)calloc(fl->num_features, sizeof(int));
        if (fl->fids == NULL) goto error_exit;
        fl->num_features = 0;
    }
    fl = &trainer->bos_trans;
    fl->fids = (int*)calloc(fl->num_features, sizeof(int));
    if (fl->fids == NULL) goto error_exit;
    fl->num_features = 0;
    fl = &trainer->eos_trans;
    fl->fids = (int*)calloc(fl->num_features, sizeof(int));
    if (fl->fids == NULL) goto error_exit;
    fl->num_features = 0;

    /*
        At last, store the feature indices.
     */
    for (k = 0;k < K;++k) {
        const crf1ml_feature_t *f = &features[k];
        switch (f->type) {
        case FT_STATE:
            fl = &trainer->attributes[f->src];
            fl->fids[fl->num_features++] = k;
            break;
        case FT_TRANS:
            fl = &trainer->forward_trans[f->src];
            fl->fids[fl->num_features++] = k;
            fl = &trainer->backward_trans[f->dst];
            fl->fids[fl->num_features++] = k;
            break;
        case FT_TRANS_BOS:
            fl = &trainer->bos_trans;
            fl->fids[fl->num_features++] = k;
            break;
        case FT_TRANS_EOS:
            fl = &trainer->eos_trans;
            fl->fids[fl->num_features++] = k;
            break;
        }
    }

    return 0;

error_exit:
    if (trainer->attributes == NULL) {
        for (i = 0;i < A;++i) free(trainer->attributes[i].fids);
        free(trainer->attributes);
        trainer->attributes = NULL;
    }
    if (trainer->forward_trans == NULL) {
        for (i = 0;i < L;++i) free(trainer->forward_trans[i].fids);
        free(trainer->forward_trans);
        trainer->forward_trans = NULL;
    }
    if (trainer->backward_trans == NULL) {
        for (i = 0;i < L;++i) free(trainer->backward_trans[i].fids);
        free(trainer->backward_trans);
        trainer->backward_trans = NULL;
    }
    return -1;
}

int crf1ml_prepare(
    crf1ml_t* trainer,
    int num_labels,
    int num_attributes,
    int max_item_length,
    crf1ml_features_t* features
    )
{
    int ret = 0;
    const int L = num_labels;
    const int A = num_attributes;
    const int T = max_item_length;

    /* Set basic parameters. */
    trainer->num_labels = L;
    trainer->num_attributes = A;

    /* Construct a CRF context. */
    trainer->ctx = crf1mc_new(L, T);
    if (trainer->ctx == NULL) {
        ret = CRFERR_OUTOFMEMORY;
        goto error_exit;
    }

    /* Initialization for features and their weights. */
    trainer->features = features->features;
    trainer->num_features = features->num_features;
    trainer->w = (floatval_t*)calloc(trainer->num_features, sizeof(floatval_t));
    if (trainer->w == NULL) {
        ret = CRFERR_OUTOFMEMORY;
        goto error_exit;
    }

    /* Allocate the work space for probability calculation. */
    trainer->prob = (floatval_t*)calloc(L, sizeof(floatval_t));
    if (trainer->prob == NULL) {
        ret = CRFERR_OUTOFMEMORY;
        goto error_exit;
    }

    /* Initialize the feature references. */
    init_feature_references(trainer, A, L);

    return ret;

error_exit:
    free(trainer->attributes);
    free(trainer->forward_trans);
    free(trainer->backward_trans);
    free(trainer->prob);
    free(trainer->ctx);
    return 0;
}

static int crf1ml_exchange_options(crf_params_t* params, crf1ml_option_t* opt, int mode)
{
    BEGIN_PARAM_MAP(params, mode)
        DDX_PARAM_STRING(
            "algorithm", opt->algorithm, "lbfgs",
            "The training algorithm."
            )
        DDX_PARAM_FLOAT(
            "feature.minfreq", opt->feature_minfreq, 0.0,
            "The minimum frequency of features."
            )
        DDX_PARAM_INT(
            "feature.possible_states", opt->feature_possible_states, 0,
            "Force to generate possible state features."
            )
        DDX_PARAM_INT(
            "feature.possible_transitions", opt->feature_possible_transitions, 0,
            "Force to generate possible transition features."
            )
    END_PARAM_MAP()

    crf1ml_lbfgs_options(params, opt, mode);
    crf1ml_sgd_options(params, opt, mode);

    return 0;
}

void crf1ml_shuffle(int *perm, int N, int init)
{
    int i, j, tmp;

    if (init) {
        /* Initialize the permutation if necessary. */
        for (i = 0;i < N;++i) {
            perm[i] = i;
        }
    }

    for (i = 0;i < N;++i) {
        j = mt_genrand_int31() % N;
        tmp = perm[j];
        perm[j] = perm[i];
        perm[i] = tmp;
    }
}

crf1ml_t* crf1ml_new()
{
#if 0
    crf1mc_test_context(stdout);
    return NULL;
#else
    crf1ml_t* trainer = (crf1ml_t*)calloc(1, sizeof(crf1ml_t));
    trainer->lg = (logging_t*)calloc(1, sizeof(logging_t));

    /* Create an instance for CRF parameters. */
    trainer->params = params_create_instance();
    /* Set the default parameters. */
    crf1ml_exchange_options(trainer->params, &trainer->opt, 0);

    return trainer;
#endif
}

void crf1ml_delete(crf1ml_t* trainer)
{
    if (trainer != NULL) {
        free(trainer->lg);
    }
}

int crf_train_tag(crf_tagger_t* tagger, crf_sequence_t *inst, crf_output_t* output)
{
    int i;
    floatval_t logscore = 0;
    crf1ml_t *crf1mt = (crf1ml_t*)tagger->internal;
    const floatval_t* w = crf1mt->w;
    const int K = crf1mt->num_features;
    crf1m_context_t* ctx = crf1mt->ctx;

    crf1mc_set_num_items(ctx, inst->num_items);

    crf1ml_transition_score(crf1mt, w, K, 1.0);
    crf1ml_set_labels(crf1mt, inst);
    crf1ml_state_score(crf1mt, inst, w, K, 1.0);
    logscore = crf1mc_viterbi(crf1mt->ctx);

    crf_output_init_n(output, inst->num_items);
    output->probability = logscore;
    for (i = 0;i < inst->num_items;++i) {
        output->labels[i] = crf1mt->ctx->labels[i];
    }
    output->num_labels = inst->num_items;

    return 0;
}





void crf_train_set_message_callback(crf_trainer_t* trainer, void *instance, crf_logging_callback cbm)
{
    crf1ml_t *crf1mt = (crf1ml_t*)trainer->internal;
    crf1mt->lg->func = cbm;
    crf1mt->lg->instance = instance;
}

void crf_train_set_evaluate_callback(crf_trainer_t* trainer, void *instance, crf_evaluate_callback cbe)
{
    crf1ml_t *crf1mt = (crf1ml_t*)trainer->internal;
    crf1mt->cbe_instance = instance;
    crf1mt->cbe_proc = cbe;
}

static int crf_train_train(
    crf_trainer_t* trainer,
    void* instances,
    int num_instances,
    int num_labels,
    int num_attributes
    )
{
    int i, max_item_length;
    int ret = 0;
    floatval_t sigma = 10, *best_w = NULL;
    crf_sequence_t* seqs = (crf_sequence_t*)instances;
    crf1ml_features_t* features = NULL;
    crf1ml_t *crf1mt = (crf1ml_t*)trainer->internal;
    crf_params_t *params = crf1mt->params;
    crf1ml_option_t *opt = &crf1mt->opt;

    /* Obtain the maximum number of items. */
    max_item_length = 0;
    for (i = 0;i < num_instances;++i) {
        if (max_item_length < seqs[i].num_items) {
            max_item_length = seqs[i].num_items;
        }
    }

    /* Access parameters. */
    crf1ml_exchange_options(crf1mt->params, opt, -1);

    /* Report the parameters. */
    logging(crf1mt->lg, "Training first-order linear-chain CRFs (trainer.crf1m)\n");
    logging(crf1mt->lg, "\n");

    /* Generate features. */
    logging(crf1mt->lg, "Feature generation\n");
    logging(crf1mt->lg, "feature.minfreq: %f\n", opt->feature_minfreq);
    logging(crf1mt->lg, "feature.possible_states: %d\n", opt->feature_possible_states);
    logging(crf1mt->lg, "feature.possible_transitions: %d\n", opt->feature_possible_transitions);
    crf1mt->clk_begin = clock();
    features = crf1ml_generate_features(
        seqs,
        num_instances,
        num_labels,
        num_attributes,
        opt->feature_possible_states ? 1 : 0,
        opt->feature_possible_transitions ? 1 : 0,
        opt->feature_minfreq,
        crf1mt->lg->func,
        crf1mt->lg->instance
        );
    logging(crf1mt->lg, "Number of features: %d\n", features->num_features);
    logging(crf1mt->lg, "Seconds required: %.3f\n", (clock() - crf1mt->clk_begin) / (double)CLOCKS_PER_SEC);
    logging(crf1mt->lg, "\n");

    /* Preparation for training. */
    crf1ml_prepare(crf1mt, num_labels, num_attributes, max_item_length, features);
    crf1mt->num_attributes = num_attributes;
    crf1mt->num_labels = num_labels;
    crf1mt->num_sequences = num_instances;
    crf1mt->seqs = seqs;

    crf1mt->tagger.internal = crf1mt;
    crf1mt->tagger.tag = crf_train_tag;

    if (strcmp(opt->algorithm, "lbfgs") == 0) {
        ret = crf1ml_lbfgs(crf1mt, opt);
    } else if (strcmp(opt->algorithm, "sgd") == 0) {
        ret = crf1ml_sgd(crf1mt, opt);
    } else {
        return CRFERR_INTERNAL_LOGIC;
    }

    return ret;
}

/*#define    CRF_TRAIN_SAVE_NO_PRUNING    1*/

static int crf_train_save(crf_trainer_t* trainer, const char *filename, crf_dictionary_t* attrs, crf_dictionary_t* labels)
{
    crf1ml_t *crf1mt = (crf1ml_t*)trainer->internal;
    int a, k, l, ret;
    int *fmap = NULL, *amap = NULL;
    crf1mmw_t* writer = NULL;
    const feature_refs_t *edge = NULL, *attr = NULL;
    const floatval_t *w = crf1mt->w;
    const floatval_t threshold = 0.01;
    const int L = crf1mt->num_labels;
    const int A = crf1mt->num_attributes;
    const int K = crf1mt->num_features;
    int J = 0, B = 0;

    /* Start storing the model. */
    logging(crf1mt->lg, "Storing the model\n");
    crf1mt->clk_begin = clock();

    /* Allocate and initialize the feature mapping. */
    fmap = (int*)calloc(K, sizeof(int));
    if (fmap == NULL) {
        goto error_exit;
    }
#ifdef    CRF_TRAIN_SAVE_NO_PRUNING
    for (k = 0;k < K;++k) fmap[k] = k;
    J = K;
#else
    for (k = 0;k < K;++k) fmap[k] = -1;
#endif/*CRF_TRAIN_SAVE_NO_PRUNING*/

    /* Allocate and initialize the attribute mapping. */
    amap = (int*)calloc(A, sizeof(int));
    if (amap == NULL) {
        goto error_exit;
    }
#ifdef    CRF_TRAIN_SAVE_NO_PRUNING
    for (a = 0;a < A;++a) amap[a] = a;
    B = A;
#else
    for (a = 0;a < A;++a) amap[a] = -1;
#endif/*CRF_TRAIN_SAVE_NO_PRUNING*/

    /*
     *    Open a model writer.
     */
    writer = crf1mmw(filename);
    if (writer == NULL) {
        goto error_exit;
    }

    /* Open a feature chunk in the model file. */
    if (ret = crf1mmw_open_features(writer)) {
        goto error_exit;
    }

    /* Determine a set of active features and attributes. */
    for (k = 0;k < crf1mt->num_features;++k) {
        crf1ml_feature_t* f = &crf1mt->features[k];
        if (w[k] != 0) {
            int src;
            crf1mm_feature_t feat;

#ifndef    CRF_TRAIN_SAVE_NO_PRUNING
            /* The feature (#k) will have a new feature id (#J). */
            fmap[k] = J++;        /* Feature #k -> #fmap[k]. */

            /* Map the source of the field. */
            if (f->type == FT_STATE) {
                /* The attribute #(f->src) will have a new attribute id (#B). */
                if (amap[f->src] < 0) amap[f->src] = B++;    /* Attribute #a -> #amap[a]. */
                src = amap[f->src];
            } else {
                src = f->src;
            }
#endif/*CRF_TRAIN_SAVE_NO_PRUNING*/

            feat.type = f->type;
            feat.src = src;
            feat.dst = f->dst;
            feat.weight = w[k];

            /* Write the feature. */
            if (ret = crf1mmw_put_feature(writer, fmap[k], &feat)) {
                goto error_exit;
            }
        }
    }

    /* Close the feature chunk. */
    if (ret = crf1mmw_close_features(writer)) {
        goto error_exit;
    }

    logging(crf1mt->lg, "Number of active features: %d (%d)\n", J, K);
    logging(crf1mt->lg, "Number of active attributes: %d (%d)\n", B, A);
    logging(crf1mt->lg, "Number of active labels: %d (%d)\n", L, L);

    /* Write labels. */
    logging(crf1mt->lg, "Writing labels\n", L);
    if (ret = crf1mmw_open_labels(writer, L)) {
        goto error_exit;
    }
    for (l = 0;l < L;++l) {
        const char *str = NULL;
        labels->to_string(labels, l, &str);
        if (str != NULL) {
            if (ret = crf1mmw_put_label(writer, l, str)) {
                goto error_exit;
            }
            labels->free(labels, str);
        }
    }
    if (ret = crf1mmw_close_labels(writer)) {
        goto error_exit;
    }

    /* Write attributes. */
    logging(crf1mt->lg, "Writing attributes\n");
    if (ret = crf1mmw_open_attrs(writer, B)) {
        goto error_exit;
    }
    for (a = 0;a < A;++a) {
        if (0 <= amap[a]) {
            const char *str = NULL;
            attrs->to_string(attrs, a, &str);
            if (str != NULL) {
                if (ret = crf1mmw_put_attr(writer, amap[a], str)) {
                    goto error_exit;
                }
                attrs->free(attrs, str);
            }
        }
    }
    if (ret = crf1mmw_close_attrs(writer)) {
        goto error_exit;
    }

    /* Write label feature references. */
    logging(crf1mt->lg, "Writing feature references for transitions\n");
    if (ret = crf1mmw_open_labelrefs(writer, L+2)) {
        goto error_exit;
    }
    for (l = 0;l < L;++l) {
        edge = TRANSITION_FROM(crf1mt, l);
        if (ret = crf1mmw_put_labelref(writer, l, edge, fmap)) {
            goto error_exit;
        }
    }
    edge = TRANSITION_BOS(crf1mt);
    if (ret = crf1mmw_put_labelref(writer, L, edge, fmap)) {
        goto error_exit;
    }
    edge = TRANSITION_EOS(crf1mt);
    if (ret = crf1mmw_put_labelref(writer, L+1, edge, fmap)) {
        goto error_exit;
    }
    if (ret = crf1mmw_close_labelrefs(writer)) {
        goto error_exit;
    }

    /* Write attribute feature references. */
    logging(crf1mt->lg, "Writing feature references for attributes\n");
    if (ret = crf1mmw_open_attrrefs(writer, B)) {
        goto error_exit;
    }
    for (a = 0;a < A;++a) {
        if (0 <= amap[a]) {
            attr = ATTRIBUTE(crf1mt, a);
            if (ret = crf1mmw_put_attrref(writer, amap[a], attr, fmap)) {
                goto error_exit;
            }
        }
    }
    if (ret = crf1mmw_close_attrrefs(writer)) {
        goto error_exit;
    }

    /* Close the writer. */
    crf1mmw_close(writer);
    logging(crf1mt->lg, "Seconds required: %.3f\n", (clock() - crf1mt->clk_begin) / (double)CLOCKS_PER_SEC);
    logging(crf1mt->lg, "\n");

    free(amap);
    free(fmap);
    return 0;

error_exit:
    if (writer != NULL) {
        crf1mmw_close(writer);
    }
    if (amap != NULL) {
        free(amap);
    }
    if (fmap != NULL) {
        free(fmap);
    }
    return ret;
}

static int crf_train_addref(crf_trainer_t* trainer)
{
    return crf_interlocked_increment(&trainer->nref);
}

static int crf_train_release(crf_trainer_t* trainer)
{
    int count = crf_interlocked_decrement(&trainer->nref);
    if (count == 0) {
    }
    return count;
}

static crf_params_t* crf_train_params(crf_trainer_t* trainer)
{
    crf1ml_t *crf1mt = (crf1ml_t*)trainer->internal;
    crf_params_t* params = crf1mt->params;
    params->addref(params);
    return params;
}


int crf1ml_create_instance(const char *interface, void **ptr)
{
    if (strcmp(interface, "trainer.crf1m") == 0) {
        crf_trainer_t* trainer = (crf_trainer_t*)calloc(1, sizeof(crf_trainer_t));

        trainer->nref = 1;
        trainer->addref = crf_train_addref;
        trainer->release = crf_train_release;

        trainer->params = crf_train_params;
    
        trainer->set_message_callback = crf_train_set_message_callback;
        trainer->set_evaluate_callback = crf_train_set_evaluate_callback;
        trainer->train = crf_train_train;
        trainer->save = crf_train_save;
        trainer->internal = crf1ml_new();

        *ptr = trainer;
        return 0;
    } else {
        return 1;
    }
}
