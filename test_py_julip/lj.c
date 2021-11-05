#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <julia.h>

#include "lj.h"


void println() { printf("\n"); }

jl_function_t* _atoms_from_c; 

jl_value_t* _cace_refs;
jl_function_t* _setindex;
jl_function_t* _delete;
jl_datatype_t* _reft;


void save_ref(jl_value_t* var) {
   JL_GC_PUSH1(&var);
   jl_value_t* rvar = jl_new_struct(_reft, var);
   jl_call3(_setindex, _cace_refs, rvar, rvar);
   JL_GC_POP();
}

void delete_ref(jl_value_t* var) {
   JL_GC_PUSH1(&var);
   jl_value_t* rvar = jl_new_struct(_reft, var);
   jl_call2(_delete, _cace_refs, rvar);
   JL_GC_POP();
}

// jl_value_t *_calculators;

// jl_value_t **_GC;

void julip_init() {
   jl_init();
   jl_eval_string("using Pkg; Pkg.activate(\".\"); Pkg.instantiate(; verbose=true);");
   jl_eval_string("using JuLIP");
   _cace_refs = jl_eval_string("__cace_refs__ = IdDict()");
   _setindex = jl_get_function(jl_base_module, "setindex!");
   _reft = (jl_datatype_t*)jl_eval_string("Base.RefValue{Any}");
   _delete = jl_get_function(jl_base_module, "delete!");
   _atoms_from_c = jl_eval_string("(X, Z, cell, bc) -> Atoms(X = X, Z = Z, cell=cell, pbc = Bool.(bc))");
   // _calculators = jl_eval_string("Dict{String, Any}()")
   return; 
}

void julip_cleanup() {
  jl_atexit_hook(0);
}


void* init_lj(char* idstr) {
   jl_value_t* calc = jl_eval_string("LennardJones() * SplineCutoff(15.0, 20.0)");
   jl_set_global(jl_main_module, jl_symbol(idstr), calc);
   return (void*) calc;
}

void* init_calculator(char* idstr, char* initcmd) {
   jl_value_t* calc = jl_eval_string(initcmd);
   jl_set_global(jl_main_module, jl_symbol(idstr), calc);
   return (void*) calc;
}

// void* load_calculator(char* idstr, char* fpath) {
//    jl_value_t* calc = jl_eval_string(initcmd);
//    jl_set_global(jl_main_module, jl_symbol(idstr), calc);
//    return (void*) calc;
// }



jl_value_t* atoms_from_c(double* X, 
                         int32_t* Z, 
                         double* cell, 
                         int32_t* bc, 
                         int Nat) {

   jl_value_t* jl_float64 = jl_apply_array_type((jl_value_t*)jl_float64_type, 1);
   jl_value_t* jl_int32 = jl_apply_array_type((jl_value_t*)jl_int32_type, 1);
   jl_array_t* _X = jl_alloc_array_1d(jl_float64, 3 * Nat);
   jl_array_t* _cell = jl_alloc_array_1d(jl_float64, 9);
   jl_array_t* _bc = jl_alloc_array_1d(jl_int32, 3);
   jl_array_t* _Z = jl_alloc_array_1d(jl_int32, Nat);

   double *XData = (double*)jl_array_data(_X);
   for (int i = 0; i < 3*Nat; i++) XData[i] = X[i];
   double *cellData = (double*)jl_array_data(_cell);
   for (int i = 0; i < 9; i++) cellData[i] = cell[i]; 
   int32_t *bcData = (int32_t*)jl_array_data(_bc);
   for (int i = 0; i < 3; i++) bcData[i] = bc[i]; 
   int32_t *ZData = (int32_t*)jl_array_data(_Z);
   for (int i = 0; i < Nat; i++) ZData[i] = Z[i]; 

   jl_value_t* at_args[] = {(jl_value_t*)_X, (jl_value_t*)_Z, (jl_value_t*)_cell, (jl_value_t*)_bc};   
   return jl_call(_atoms_from_c, at_args, 4);
}


double unbox_float64(jl_value_t* jlval) {
   if jl_typeis(jlval, jl_float64_type) {
      return jl_unbox_float64(jlval);
   } 
   jl_errorf("Can't identify the return type!!! \n");
   return 0.0;
}

double energy(char* calcid, double* X, int32_t* Z, double* cell, int32_t* pbc, int Nat){

   jl_value_t** args; 
   JL_GC_PUSHARGS(args, 2);

   jl_value_t* calc = jl_eval_string(calcid);
   args[0] = calc; 
   jl_value_t* at = atoms_from_c(X, Z, cell, pbc, Nat);
   args[1] = at;

   jl_value_t *energyfcn = jl_get_function(jl_main_module, "energy");
   jl_value_t* jlE = jl_call2(energyfcn, calc, at);   

   JL_GC_POP();

   return unbox_float64(jlE);
}




void forces(char* calcid, double *F, 
              double* X, int32_t* Z, double* cell, int32_t* pbc, int Nat){
  
   jl_value_t** args; 
   JL_GC_PUSHARGS(args, 2);


   jl_value_t* calc = jl_eval_string(calcid);
   args[0] = calc; 
   jl_value_t* at = atoms_from_c(X, Z, cell, pbc, Nat);
   args[1] = at;

   jl_value_t *forcefcn = jl_eval_string("(calc, at) -> mat(forces(calc, at))[:]");
   jl_array_t* _F = (jl_array_t*)jl_call2(forcefcn, calc, at);
   double *Fdata = (double*)jl_array_data(_F);
   for (int i = 0; i < 3*Nat; i++) F[i] = Fdata[i];

   JL_GC_POP();

   return; 
}