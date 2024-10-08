/*********************************************************************
 **
 ** file: rma_PLM.c
 **
 ** Aim: fit the RMA model as a PLMset object.
 **
 ** Copyright (C) 2003 Ben Bolstad
 **
 ** created by: B. M. Bolstad <bmb@bmbolstad.com>
 ** 
 ** created on: Sep 14, 2003
 **
 ** Last modified: Sept 14, 2003
 **
 ** Modification history
 ** Sept 14, 2003 - Intial version
 ** May 3, 2004   - Fixed a subtle and small memory leak.
 ** March 1, 2006 - change all comments to ansi c standard
 ** Oct 10, 2006 - add verbosity arguments to pp_* calls
 ** Oct 11, 2006 - make verbosity argument get passed to summarization function 
 ** Jan 6, 2009 - change SET_VECTOR_ELT to SET_STRING_ELT where relevant.
 **
 *********************************************************************/

#include "preprocess.h"
#include "do_PLMrma.h"
#include "common_types.h"

#include <R.h>
#include <Rdefines.h>
#include <Rmath.h>
#include <Rinternals.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>





/*********************************************************************
 **
 ** static void rmaPLM_alloc_space(PLMRoutput *Routput, PLMoutput *output,
 **                                outputsettings *store,Datagroup *data, 
 **                                PLMmodelparam *model)
 **
 ** 
 ** This function allocates all the space that is needed for storing
 ** user desired output from the PLM
 ** 
 **
 ********************************************************************/

static void rmaPLM_alloc_space(PLMRoutput *Routput, PLMoutput *output,outputsettings *store,Datagroup *data, PLMmodelparam *model){

   
  Routput->nprotected = 0;

  
  output->outnames = (char **)R_Calloc(data->nprobesets,char *);

  if (store->weights){
    PROTECT(Routput->weights = allocMatrix(REALSXP, data->rows, data->cols));
  } else {
    PROTECT(Routput->weights = allocMatrix(REALSXP, 0, 0));
  }
  Routput->nprotected++;
  output->out_weights = NUMERIC_POINTER(Routput->weights);


  PROTECT(Routput->probe_coef = allocMatrix(REALSXP,data->rows,1));
  Routput->nprotected++;
  output->out_probeparams = NUMERIC_POINTER(Routput->probe_coef);


  PROTECT(Routput->chip_coef = allocMatrix(REALSXP, data->nprobesets, model->nchipparams));
  Routput->nprotected++;
  output->out_chipparams = NUMERIC_POINTER(Routput->chip_coef);
  
  PROTECT(Routput->const_coef = allocMatrix(REALSXP, data->nprobesets, 1));
  Routput->nprotected++;
  output->out_constparams = NUMERIC_POINTER(Routput->const_coef);

  PROTECT(Routput->chip_SE = allocMatrix(REALSXP, data->nprobesets, model->nchipparams));
  Routput->nprotected++;
  output->out_chip_SE = NUMERIC_POINTER(Routput->chip_SE);


  PROTECT(Routput->probe_SE = allocMatrix(REALSXP,data->rows,1));
  Routput->nprotected++;
  output->out_probe_SE = NUMERIC_POINTER(Routput->probe_SE);


  PROTECT(Routput->const_SE = allocMatrix(REALSXP, data->nprobesets, 1));
  Routput->nprotected++;
  output->out_const_SE = NUMERIC_POINTER(Routput->const_SE);


  if (store->residuals){
    PROTECT(Routput->residuals = allocMatrix(REALSXP, data->rows, data->cols));
  } else {
    PROTECT(Routput->residuals = allocMatrix(REALSXP, 0, 0));
  }
  Routput->nprotected++;
  output->out_resids = NUMERIC_POINTER(Routput->residuals); 
  
  /* Not returned for RMA */
  PROTECT(Routput->residSE = allocMatrix(REALSXP,0,0));
  
  Routput->nprotected++;
  output->out_residSE = NUMERIC_POINTER(Routput->residSE);

  
  /* variance covariance is never returned by median polish (at least intially, later we might add in a pseudo varcov */
  PROTECT(Routput->varcov = allocVector(VECSXP,0));
  output->out_varcov= NULL;
  Routput->nprotected++;
  
}


/*********************************************************************
 **
 ** SEXP rmaPLMset(SEXP PMmat, SEXP MMmat, SEXP ProbeNamesVec,SEXP N_probes, SEXP outputparam, SEXP modelparam)
 **
 **
 *********************************************************************/

SEXP rmaPLMset(SEXP PMmat, SEXP MMmat, SEXP ProbeNamesVec,SEXP N_probes, SEXP outputparam, SEXP modelparam, SEXP verbosity){

  int i;

  int verbosity_level;
  
  outputsettings *store = (outputsettings *)R_Calloc(1,outputsettings);
  Datagroup *data = (Datagroup *)R_Calloc(1,Datagroup);
  PLMoutput *output = (PLMoutput *)R_Calloc(1,PLMoutput);
  PLMmodelparam *model = (PLMmodelparam *)R_Calloc(1,PLMmodelparam);
  PLMRoutput *Routput = (PLMRoutput *)R_Calloc(1,PLMRoutput);
  
  SEXP dim1;
  SEXP dimnames,names;
  SEXP output_list;
  SEXP param;



  verbosity_level = asInteger(verbosity);


  /* organise data to be passed to model fitting routines */
  
  PROTECT(dim1 = getAttrib(PMmat,R_DimSymbol));
  
  data->rows = INTEGER(dim1)[0];
  data->cols = INTEGER(dim1)[1];
  
  /*  PROTECT(dim2 = getAttrib(chipcovariates,R_DimSymbol)); */

  data->PM = NUMERIC_POINTER(AS_NUMERIC(PMmat));
  data->MM = NUMERIC_POINTER(AS_NUMERIC(MMmat));
  data->nprobesets = INTEGER(N_probes)[0];
  
  /* Get the names corresponding to each row */
    
  data->ProbeNames = (const char **)R_Calloc(data->rows,const char *);
  for (i =0; i < data->rows; i++){
    data->ProbeNames[i] = CHAR(STRING_ELT(ProbeNamesVec,i));
  }
  

  /* figure out what the covariate matrix should be based on the rlm_model_type information and chipcovariates */

  param = GetParameter(modelparam,"psi.type");
  model->psi_code = asInteger(param);
  /* model->psi_code = 0; */
  /*  model->method = asInteger(rlm_model_type); */
  model->method = 0;
  /* param = GetParameter(modelparam,"se.type"); */
  model->se_method = asInteger(param);
  model->se_method = 0;
  param = GetParameter(modelparam,"psi.k");
  model->psi_k = asReal(param);
  /* model->psi_k =0; */
  /*  model->input_chipcovariates = NUMERIC_POINTER(chipcovariates); */
  model->input_chipcovariates = 0;
  /*  model->nchipparams = INTEGER(dim2)[1]; */

  model->nchipparams = data->cols;


  /*  //param = GetParameter(modelparam,"max.its");
  ** //model->n_rlm_iterations = asInteger(param);
  ** //param = GetParameter(modelparam,"init.method");
  ** //if (strcmp(CHAR(VECTOR_ELT(param,0)),"ls") == 0){
  ** //  model->init_method = 0;
  ** //} else if (strcmp(CHAR(VECTOR_ELT(param,0)),"median.polish") == 0){
  ** //  model->init_method = 1;
  ** //} else if (strcmp(CHAR(VECTOR_ELT(param,0)),"Huber") == 0){
  ** // model->init_method = 2;
  ** //}
  */
  model->init_method = 0;


  /* figure out what optional features we want to output */
  param = GetParameter(outputparam,"weights");
  store->weights = asInteger(param);
  param = GetParameter(outputparam,"residuals");
  store->residuals = asInteger(param);
  param = GetParameter(outputparam,"pseudo.SE");
  store->pseudoSE = asInteger(param);
  store->residSE = 0;
  /* param = GetParameter(outputparam,"varcov"); */
  store->varcov = 0;
  

  /* Make space for output */
  rmaPLM_alloc_space(Routput, output, store, data, model);
  
  

  /* now go actually fit the model */
  if (verbosity_level > 0){
    Rprintf("Fitting models\n");
  }

  do_PLMrma(data, model, output,store);
  
  /* now lets put names on the output matrices */
  /* First the chip coef matrix */
  PROTECT(dimnames = allocVector(VECSXP,2));
  PROTECT(names = allocVector(STRSXP,data->nprobesets));
  for ( i =0; i < data->nprobesets; i++)
    SET_STRING_ELT(names,i,mkChar(output->outnames[i]));
  SET_VECTOR_ELT(dimnames,0,names);
  setAttrib(Routput->chip_coef, R_DimNamesSymbol, dimnames); 
  setAttrib(Routput->chip_SE,R_DimNamesSymbol, dimnames);

  
  /* Now the probe coef matrix */
  /* setAttrib(probe_coef, R_DimNamesSymbol, dimnames); */
  /* setAttrib(probe_SE, R_DimNamesSymbol, dimnames); */
   
  /*Now lets create the output_list */

  PROTECT(output_list = allocVector(VECSXP,10));

  SET_VECTOR_ELT(output_list,0,Routput->chip_coef);
  SET_VECTOR_ELT(output_list,1,Routput->probe_coef);
  SET_VECTOR_ELT(output_list,2,Routput->weights);
  SET_VECTOR_ELT(output_list,3,Routput->chip_SE);
  SET_VECTOR_ELT(output_list,4,Routput->probe_SE);
  SET_VECTOR_ELT(output_list,5,Routput->const_coef);
  SET_VECTOR_ELT(output_list,6,Routput->const_SE);
  SET_VECTOR_ELT(output_list,7,Routput->residuals);
  SET_VECTOR_ELT(output_list,8,Routput->residSE);
  SET_VECTOR_ELT(output_list,9,Routput->varcov);
  UNPROTECT(Routput->nprotected + 4);

  for ( i =0; i < data->nprobesets; i++)
    R_Free(output->outnames[i]); 
  R_Free(output->outnames);
  R_Free(data->ProbeNames);
  R_Free(data);
  R_Free(output);
  R_Free(Routput);
  R_Free(store);
  R_Free(model);
  
  return output_list;
  
}



/*********************************************************************
 **
 ** SEXP R_rmaPLMset_c(SEXP PMmat, SEXP MMmat, SEXP ProbeNamesVec,SEXP N_probes,SEXP densfunc, 
 **                    SEXP rho,SEXP norm_flag, SEXP bg_flag, SEXP bg_type,SEXP norm_type, SEXP summary_type)
 **
 ********************************************************************/

SEXP R_rmaPLMset_c(SEXP PMmat, SEXP MMmat, SEXP ProbeNamesVec,SEXP N_probes,SEXP norm_flag, SEXP bg_flag, SEXP bg_type,SEXP norm_type, SEXP background_parameters,SEXP norm_parameters, SEXP output_parameters, SEXP model_parameters, SEXP verbosity){


  SEXP dim1,PMcopy,rmaPLMresults;
  int rows,cols;

  /*Create a copy matrix to work on. Allows us to modify data in background and normalization steps without affecting original data */
  PROTECT(dim1 = getAttrib(PMmat,R_DimSymbol));
  rows = INTEGER(dim1)[0];
  cols = INTEGER(dim1)[1];
  PROTECT(PMcopy = allocMatrix(REALSXP,rows,cols));
  copyMatrix(PMcopy,PMmat,0);

  /* If Background correction do it */
  if (INTEGER(bg_flag)[0]){
    PMcopy = pp_background(PMcopy, MMmat, ProbeNamesVec,N_probes,bg_type,background_parameters, verbosity);
  }

  /* If Normalization do it */
  if (INTEGER(norm_flag)[0]){
    PMcopy = pp_normalize(PMcopy, MMmat, ProbeNamesVec,N_probes,norm_type, norm_parameters, verbosity);
  }
  
  /* Now do RLM fit */

  rmaPLMresults = rmaPLMset(PMcopy, MMmat, ProbeNamesVec, N_probes, output_parameters,model_parameters, verbosity);
  
  UNPROTECT(2);

  return rmaPLMresults;
}
