/*

   BLIS
   An object-based framework for developing high-performance BLAS-like
   libraries.

   Copyright (C) 2014, The University of Texas at Austin

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    - Neither the name(s) of the copyright holder(s) nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "blis.h"

#if 0

// An implementation that attempts to facilitate emission of vectorized
// instructions via constant loop bounds + #pragma omp simd directives.

#undef  GENTFUNC
#define GENTFUNC( ctype, ch, opname, arch, suf, mr, nr ) \
\
void PASTEMAC3(ch,opname,arch,suf) \
     ( \
       ctype*     restrict a, \
       ctype*     restrict b, \
       ctype*     restrict c, inc_t rs_c, inc_t cs_c, \
       auxinfo_t* restrict data, \
       cntx_t*    restrict cntx  \
     ) \
{ \
        const inc_t     rs_a   = 1; \
        const inc_t     cs_a   = mr; \
\
        const inc_t     rs_b   = nr; \
        const inc_t     cs_b   = 1; \
\
        PRAGMA_SIMD \
        for ( dim_t i = 0; i < mr; ++i ) \
        { \
            /* b1 = b1 - a10t * B0; */ \
            /* b1 = b1 / alpha11; */ \
            for ( dim_t j = 0; j < nr; ++j ) \
            { \
                ctype beta11c = b[i*rs_b + j*cs_b]; \
                ctype rho11; \
\
                /* beta11 = beta11 - a10t * b01; */ \
                PASTEMAC(ch,set0s)( rho11 ); \
                for ( dim_t l = 0; l < i; ++l ) \
                { \
                    PASTEMAC(ch,axpys)( a[i*rs_a + l*cs_a], \
                                        b[l*rs_b + j*cs_b], rho11 ); \
                } \
                PASTEMAC(ch,subs)( rho11, beta11c ); \
\
                /* beta11 = beta11 / alpha11; */ \
                /* NOTE: The INVERSE of alpha11 (1.0/alpha11) is stored instead
                   of alpha11, so we can multiply rather than divide. We store
                   the inverse of alpha11 intentionally to avoid expensive
                   division instructions within the micro-kernel. */ \
                PASTEMAC(ch,scals)( a[i*rs_a + i*cs_a], beta11c ); \
\
                /* Output final result to matrix c. */ \
                PASTEMAC(ch,copys)( beta11c, c[i*rs_c + j*cs_c] ); \
\
                /* Store the local value back to b11. */ \
                PASTEMAC(ch,copys)( beta11c, b[i*rs_b + j*cs_b] ); \
            } \
        } \
}

//INSERT_GENTFUNC_BASIC2( trsm_l, BLIS_CNAME_INFIX, BLIS_REF_SUFFIX )
GENTFUNC( float,    s, trsm_l, BLIS_CNAME_INFIX, BLIS_REF_SUFFIX, 4, 16 )
GENTFUNC( double,   d, trsm_l, BLIS_CNAME_INFIX, BLIS_REF_SUFFIX, 4, 8 )
GENTFUNC( scomplex, c, trsm_l, BLIS_CNAME_INFIX, BLIS_REF_SUFFIX, 4, 8 )
GENTFUNC( dcomplex, z, trsm_l, BLIS_CNAME_INFIX, BLIS_REF_SUFFIX, 4, 4 )


#undef  GENTFUNC
#define GENTFUNC( ctype, ch, opname, arch, suf, mr, nr ) \
\
void PASTEMAC3(ch,opname,arch,suf) \
     ( \
       ctype*     restrict a, \
       ctype*     restrict b, \
       ctype*     restrict c, inc_t rs_c, inc_t cs_c, \
       auxinfo_t* restrict data, \
       cntx_t*    restrict cntx  \
     ) \
{ \
        const inc_t     rs_a   = 1; \
        const inc_t     cs_a   = mr; \
\
        const inc_t     rs_b   = nr; \
        const inc_t     cs_b   = 1; \
\
        PRAGMA_SIMD \
        for ( dim_t iter = 0; iter < mr; ++iter ) \
        { \
            dim_t i = mr - iter - 1; \
\
            /* b1 = b1 - a12t * B2; */ \
            /* b1 = b1 / alpha11; */ \
            for ( dim_t j = 0; j < nr; ++j ) \
            { \
                ctype beta11c = b[i*rs_b + j*cs_b]; \
                ctype rho11; \
\
                /* beta11 = beta11 - a12t * b21; */ \
                PASTEMAC(ch,set0s)( rho11 ); \
                for ( dim_t l = 0; l < iter; ++l ) \
                { \
                    PASTEMAC(ch,axpys)( a[i*rs_a + (i+1+l)*cs_a], \
                                        b[(i+1+l)*rs_b + j*cs_b], rho11 ); \
                } \
                PASTEMAC(ch,subs)( rho11, beta11c ); \
\
                /* beta11 = beta11 / alpha11; */ \
                /* NOTE: The INVERSE of alpha11 (1.0/alpha11) is stored instead
                   of alpha11, so we can multiply rather than divide. We store
                   the inverse of alpha11 intentionally to avoid expensive
                   division instructions within the micro-kernel. */ \
                PASTEMAC(ch,scals)( a[i*rs_a + i*cs_a], beta11c ); \
\
                /* Output final result to matrix c. */ \
                PASTEMAC(ch,copys)( beta11c, c[i*rs_c + j*cs_c] ); \
\
                /* Store the local value back to b11. */ \
                PASTEMAC(ch,copys)( beta11c, b[i*rs_b + j*cs_b] ); \
            } \
        } \
}

//INSERT_GENTFUNC_BASIC2( trsm_u, BLIS_CNAME_INFIX, BLIS_REF_SUFFIX )
GENTFUNC( float,    s, trsm_u, BLIS_CNAME_INFIX, BLIS_REF_SUFFIX, 4, 16 )
GENTFUNC( double,   d, trsm_u, BLIS_CNAME_INFIX, BLIS_REF_SUFFIX, 4, 8 )
GENTFUNC( scomplex, c, trsm_u, BLIS_CNAME_INFIX, BLIS_REF_SUFFIX, 4, 8 )
GENTFUNC( dcomplex, z, trsm_u, BLIS_CNAME_INFIX, BLIS_REF_SUFFIX, 4, 4 )

#else

// An implementation that uses variable loop bounds (queried from the context)
// and makes no use of #pragma omp simd.
#ifdef BLIS_ENABLE_TRSM_PREINVERSION

#undef  GENTFUNC
#define GENTFUNC( ctype, ch, opname, arch, suf ) \
\
void PASTEMAC3(ch,opname,arch,suf) \
     ( \
       ctype*     restrict a, \
       ctype*     restrict b, \
       ctype*     restrict c, inc_t rs_c, inc_t cs_c, \
       auxinfo_t* restrict data, \
       cntx_t*    restrict cntx  \
     ) \
{ \
        const num_t     dt     = PASTEMAC(ch,type); \
\
        const dim_t     mr     = bli_cntx_get_blksz_def_dt( dt, BLIS_MR, cntx ); \
        const dim_t     nr     = bli_cntx_get_blksz_def_dt( dt, BLIS_NR, cntx ); \
\
        const inc_t     packmr = bli_cntx_get_blksz_max_dt( dt, BLIS_MR, cntx ); \
        const inc_t     packnr = bli_cntx_get_blksz_max_dt( dt, BLIS_NR, cntx ); \
\
        const dim_t     m      = mr; \
        const dim_t     n      = nr; \
\
        const inc_t     rs_a   = 1; \
        const inc_t     cs_a   = packmr; \
\
        const inc_t     rs_b   = packnr; \
        const inc_t     cs_b   = 1; \
\
        dim_t           iter, i, j, l; \
        dim_t           n_behind; \
\
        for ( iter = 0; iter < m; ++iter ) \
        { \
            i        = iter; \
            n_behind = i; \
\
            ctype* restrict alpha11  = a + (i  )*rs_a + (i  )*cs_a; \
            ctype* restrict a10t     = a + (i  )*rs_a + (0  )*cs_a; \
            ctype* restrict B0       = b + (0  )*rs_b + (0  )*cs_b; \
            ctype* restrict b1       = b + (i  )*rs_b + (0  )*cs_b; \
\
            /* b1 = b1 - a10t * B0; */ \
            /* b1 = b1 / alpha11; */ \
            for ( j = 0; j < n; ++j ) \
            { \
                ctype* restrict b01     = B0 + (0  )*rs_b + (j  )*cs_b; \
                ctype* restrict beta11  = b1 + (0  )*rs_b + (j  )*cs_b; \
                ctype* restrict gamma11 = c  + (i  )*rs_c + (j  )*cs_c; \
                ctype           beta11c = *beta11; \
                ctype           rho11; \
\
                /* beta11 = beta11 - a10t * b01; */ \
                PASTEMAC(ch,set0s)( rho11 ); \
                for ( l = 0; l < n_behind; ++l ) \
                { \
                    ctype* restrict alpha10 = a10t + (l  )*cs_a; \
                    ctype* restrict beta01  = b01  + (l  )*rs_b; \
\
                    PASTEMAC(ch,axpys)( *alpha10, *beta01, rho11 ); \
                } \
                PASTEMAC(ch,subs)( rho11, beta11c ); \
\
                /* beta11 = beta11 / alpha11; */ \
                /* NOTE: The INVERSE of alpha11 (1.0/alpha11) is stored instead
                   of alpha11, so we can multiply rather than divide. We store
                   the inverse of alpha11 intentionally to avoid expensive
                   division instructions within the micro-kernel. */ \
                PASTEMAC(ch,scals)( *alpha11, beta11c ); \
\
                /* Output final result to matrix c. */ \
                PASTEMAC(ch,copys)( beta11c, *gamma11 ); \
\
                /* Store the local value back to b11. */ \
                PASTEMAC(ch,copys)( beta11c, *beta11 ); \
            } \
        } \
}

INSERT_GENTFUNC_BASIC2( trsm_l, BLIS_CNAME_INFIX, BLIS_REF_SUFFIX )


#undef  GENTFUNC
#define GENTFUNC( ctype, ch, opname, arch, suf ) \
\
void PASTEMAC3(ch,opname,arch,suf) \
     ( \
       ctype*     restrict a, \
       ctype*     restrict b, \
       ctype*     restrict c, inc_t rs_c, inc_t cs_c, \
       auxinfo_t* restrict data, \
       cntx_t*    restrict cntx  \
     ) \
{ \
        const num_t     dt     = PASTEMAC(ch,type); \
\
        const dim_t     mr     = bli_cntx_get_blksz_def_dt( dt, BLIS_MR, cntx ); \
        const dim_t     nr     = bli_cntx_get_blksz_def_dt( dt, BLIS_NR, cntx ); \
\
        const inc_t     packmr = bli_cntx_get_blksz_max_dt( dt, BLIS_MR, cntx ); \
        const inc_t     packnr = bli_cntx_get_blksz_max_dt( dt, BLIS_NR, cntx ); \
\
        const dim_t     m      = mr; \
        const dim_t     n      = nr; \
\
        const inc_t     rs_a   = 1; \
        const inc_t     cs_a   = packmr; \
\
        const inc_t     rs_b   = packnr; \
        const inc_t     cs_b   = 1; \
\
        dim_t           iter, i, j, l; \
        dim_t           n_behind; \
\
        for ( iter = 0; iter < m; ++iter ) \
        { \
            i        = m - iter - 1; \
            n_behind = iter; \
\
            ctype* restrict alpha11  = a + (i  )*rs_a + (i  )*cs_a; \
            ctype* restrict a12t     = a + (i  )*rs_a + (i+1)*cs_a; \
            ctype* restrict b1       = b + (i  )*rs_b + (0  )*cs_b; \
            ctype* restrict B2       = b + (i+1)*rs_b + (0  )*cs_b; \
\
            /* b1 = b1 - a12t * B2; */ \
            /* b1 = b1 / alpha11; */ \
            for ( j = 0; j < n; ++j ) \
            { \
                ctype* restrict beta11  = b1 + (0  )*rs_b + (j  )*cs_b; \
                ctype* restrict b21     = B2 + (0  )*rs_b + (j  )*cs_b; \
                ctype* restrict gamma11 = c  + (i  )*rs_c + (j  )*cs_c; \
                ctype           beta11c = *beta11; \
                ctype           rho11; \
\
                /* beta11 = beta11 - a12t * b21; */ \
                PASTEMAC(ch,set0s)( rho11 ); \
                for ( l = 0; l < n_behind; ++l ) \
                { \
                    ctype* restrict alpha12 = a12t + (l  )*cs_a; \
                    ctype* restrict beta21  = b21  + (l  )*rs_b; \
\
                    PASTEMAC(ch,axpys)( *alpha12, *beta21, rho11 ); \
                } \
                PASTEMAC(ch,subs)( rho11, beta11c ); \
\
                /* beta11 = beta11 / alpha11; */ \
                /* NOTE: The INVERSE of alpha11 (1.0/alpha11) is stored instead
                   of alpha11, so we can multiply rather than divide. We store
                   the inverse of alpha11 intentionally to avoid expensive
                   division instructions within the micro-kernel. */ \
                PASTEMAC(ch,scals)( *alpha11, beta11c ); \
\
                /* Output final result to matrix c. */ \
                PASTEMAC(ch,copys)( beta11c, *gamma11 ); \
\
                /* Store the local value back to b11. */ \
                PASTEMAC(ch,copys)( beta11c, *beta11 ); \
            } \
        } \
}

INSERT_GENTFUNC_BASIC2( trsm_u, BLIS_CNAME_INFIX, BLIS_REF_SUFFIX )

#endif //BLIS_ENABLE_TRSM_PREINVERSIO

#ifdef BLIS_DISABLE_TRSM_PREINVERSION
#define BLIS_FLOAT_MIN_SN   1.40129846e-45
#define BLIS_DOUBLE_MIN_SN  4.9406564584124654e-324
float smuls(float a, float b)
{
    float c = a*b;
    /*Returning minimum value when product of two non-zero 
     * numbers resulting zero due to staturation*/
    if((c==0e0)&&(a!=0e0)&&(b!=0e0)){
       return BLIS_FLOAT_MIN_SN;
    }
    return c;
}
double dmuls(double a, double b)
{
    double c = a*b;
    /*Returning minimum value when product of two non-zero 
     * numbers resulting zero due to staturation*/
    if((c==0e0)&&(a!=0e0)&&(b!=0e0)){
       return BLIS_DOUBLE_MIN_SN;
    }
    return c;
}

#undef  GENTFUNC
#define GENTFUNC( ctype, ch, opname, arch, suf ) \
\
void PASTEMAC3(ch,opname,arch,suf) \
     ( \
       ctype*     restrict a, \
       ctype*     restrict b, \
       ctype*     restrict c, inc_t rs_c, inc_t cs_c, \
       auxinfo_t* restrict data, \
       cntx_t*    restrict cntx  \
     ) \
{ \
        const num_t     dt     = PASTEMAC(ch,type); \
\
        const dim_t     mr     = bli_cntx_get_blksz_def_dt( dt, BLIS_MR, cntx ); \
        const dim_t     nr     = bli_cntx_get_blksz_def_dt( dt, BLIS_NR, cntx ); \
\
        const inc_t     packmr = bli_cntx_get_blksz_max_dt( dt, BLIS_MR, cntx ); \
        const inc_t     packnr = bli_cntx_get_blksz_max_dt( dt, BLIS_NR, cntx ); \
\
        const dim_t     m      = mr; \
        const dim_t     n      = nr; \
\
        const inc_t     rs_a   = 1; \
        const inc_t     cs_a   = packmr; \
\
        const inc_t     rs_b   = packnr; \
        const inc_t     cs_b   = 1; \
\
        dim_t           iter, i, j, l; \
        dim_t           n_behind; \
\
        for ( iter = 0; iter < m; ++iter ) \
        { \
            i        = iter; \
            n_behind = i; \
\
            ctype* restrict alpha11  = a + (i  )*rs_a + (i  )*cs_a; \
            ctype* restrict a10t     = a + (i  )*rs_a + (0  )*cs_a; \
            ctype* restrict B0       = b + (0  )*rs_b + (0  )*cs_b; \
            ctype* restrict b1       = b + (i  )*rs_b + (0  )*cs_b; \
\
            /* b1 = b1 - a10t * B0; */ \
            /* b1 = b1 / alpha11; */ \
            for ( j = 0; j < n; ++j ) \
            { \
                ctype* restrict b01     = B0 + (0  )*rs_b + (j  )*cs_b; \
                ctype* restrict beta11  = b1 + (0  )*rs_b + (j  )*cs_b; \
                ctype* restrict gamma11 = c  + (i  )*rs_c + (j  )*cs_c; \
                ctype           beta11c = *beta11; \
                ctype           rho11; \
\
                /* beta11 = beta11 - a10t * b01; */ \
                PASTEMAC(ch,set0s)( rho11 ); \
                for ( l = 0; l < n_behind; ++l ) \
                { \
                    ctype* restrict alpha10 = a10t + (l  )*cs_a; \
                    ctype* restrict beta01  = b01  + (l  )*rs_b; \
\
                    PASTEMAC(ch,axpys)( *alpha10, *beta01, rho11 ); \
                } \
                PASTEMAC(ch,subs)( rho11, beta11c ); \
\
                /* performing division based on data type 
		 * beta11 = beta11 / alpha11; */ \
                if(dt==BLIS_FLOAT) \
                { \
                    float *z = (float*)&beta11c; \
                    float *y = (float*)alpha11; \
                    float *x = (float*)&beta11c; \
                    *z = *x / *y; \
                }else if(dt==BLIS_DOUBLE) \
                { \
                    double *z = (double*)&beta11c; \
                    double *y = (double*)alpha11; \
                    double *x = (double*)&beta11c; \
                    *z = *x / *y; \
                }else if(dt==BLIS_SCOMPLEX) \
                { \
                    float *z = (float*)&beta11c; \
                    float *y = (float*)alpha11;  \
                    float *x = (float*)&beta11c; \
                    float tr,ti,td; \
                    tr = smuls(x[0],y[0]) + smuls(x[1],y[1]); \
                    td = smuls(y[0],y[0]) + smuls(y[1],y[1]); \
                    ti = smuls(x[1],y[0]) - smuls(x[0],y[1]); \
                    z[0]= (tr/td); \
                    z[1]= (ti/td); \
                }else if(dt==BLIS_DCOMPLEX) \
                { \
                    double *z = (double*)&beta11c; \
                    double *y = (double*)alpha11;  \
                    double *x = (double*)&beta11c; \
                    double tr,ti,td; \
                    tr = dmuls(x[0],y[0]) + dmuls(x[1],y[1]); \
                    td = dmuls(y[0],y[0]) + dmuls(y[1],y[1]); \
                    ti = dmuls(x[1],y[0]) - dmuls(x[0],y[1]); \
                    z[0]= (tr/td); \
                    z[1]= (ti/td); \
                } \
\
                /* Output final result to matrix c. */ \
                PASTEMAC(ch,copys)( beta11c, *gamma11 ); \
\
                /* Store the local value back to b11. */ \
                PASTEMAC(ch,copys)( beta11c, *beta11 ); \
            } \
        } \
}

INSERT_GENTFUNC_BASIC2( trsm_l, BLIS_CNAME_INFIX, BLIS_REF_SUFFIX )

#undef  GENTFUNC
#define GENTFUNC( ctype, ch, opname, arch, suf ) \
\
void PASTEMAC3(ch,opname,arch,suf) \
     ( \
       ctype*     restrict a, \
       ctype*     restrict b, \
       ctype*     restrict c, inc_t rs_c, inc_t cs_c, \
       auxinfo_t* restrict data, \
       cntx_t*    restrict cntx  \
     ) \
{ \
        const num_t     dt     = PASTEMAC(ch,type); \
\
        const dim_t     mr     = bli_cntx_get_blksz_def_dt( dt, BLIS_MR, cntx ); \
        const dim_t     nr     = bli_cntx_get_blksz_def_dt( dt, BLIS_NR, cntx ); \
\
        const inc_t     packmr = bli_cntx_get_blksz_max_dt( dt, BLIS_MR, cntx ); \
        const inc_t     packnr = bli_cntx_get_blksz_max_dt( dt, BLIS_NR, cntx ); \
\
        const dim_t     m      = mr; \
        const dim_t     n      = nr; \
\
        const inc_t     rs_a   = 1; \
        const inc_t     cs_a   = packmr; \
\
        const inc_t     rs_b   = packnr; \
        const inc_t     cs_b   = 1; \
\
        dim_t           iter, i, j, l; \
        dim_t           n_behind; \
\
        for ( iter = 0; iter < m; ++iter ) \
        { \
            i        = m - iter - 1; \
            n_behind = iter; \
\
            ctype* restrict alpha11  = a + (i  )*rs_a + (i  )*cs_a; \
            ctype* restrict a12t     = a + (i  )*rs_a + (i+1)*cs_a; \
            ctype* restrict b1       = b + (i  )*rs_b + (0  )*cs_b; \
            ctype* restrict B2       = b + (i+1)*rs_b + (0  )*cs_b; \
\
            /* b1 = b1 - a12t * B2; */ \
            /* b1 = b1 / alpha11; */ \
            for ( j = 0; j < n; ++j ) \
            { \
                ctype* restrict beta11  = b1 + (0  )*rs_b + (j  )*cs_b; \
                ctype* restrict b21     = B2 + (0  )*rs_b + (j  )*cs_b; \
                ctype* restrict gamma11 = c  + (i  )*rs_c + (j  )*cs_c; \
                ctype           beta11c = *beta11; \
                ctype           rho11; \
\
                /* beta11 = beta11 - a12t * b21; */ \
                PASTEMAC(ch,set0s)( rho11 ); \
                for ( l = 0; l < n_behind; ++l ) \
                { \
                    ctype* restrict alpha12 = a12t + (l  )*cs_a; \
                    ctype* restrict beta21  = b21  + (l  )*rs_b; \
\
                    PASTEMAC(ch,axpys)( *alpha12, *beta21, rho11 ); \
                } \
                PASTEMAC(ch,subs)( rho11, beta11c ); \
\
                /* performing division based on data type 
		 * beta11 = beta11 / alpha11; */ \
                if(dt==BLIS_FLOAT) \
                { \
                    float *z = (float*)&beta11c; \
                    float *y = (float*)alpha11; \
                    float *x = (float*)&beta11c; \
                   *z = *x / *y; \
                }else if(dt==BLIS_DOUBLE) \
                { \
                    double *z = (double*)&beta11c; \
                    double *y = (double*)alpha11; \
                    double *x = (double*)&beta11c; \
                   *z = *x / *y; \
                }else if(dt==BLIS_SCOMPLEX) \
                { \
                     float *z = (float*)&beta11c; \
                    float *y = (float*)alpha11;  \
                    float *x = (float*)&beta11c; \
                    float tr,ti,td; \
                    tr = smuls(x[0],y[0]) + smuls(x[1],y[1]); \
                    td = smuls(y[0],y[0]) + smuls(y[1],y[1]); \
                    ti = smuls(x[1],y[0]) - smuls(x[0],y[1]); \
                    z[0]= (tr/td); \
                    z[1]= (ti/td); \
                }else if(dt==BLIS_DCOMPLEX) \
                { \
                     double *z = (double*)&beta11c; \
                    double *y = (double*)alpha11;  \
                    double *x = (double*)&beta11c; \
                    double tr,ti,td; \
                    tr = dmuls(x[0],y[0]) + dmuls(x[1],y[1]); \
                    td = dmuls(y[0],y[0]) + dmuls(y[1],y[1]); \
                    ti = dmuls(x[1],y[0]) - dmuls(x[0],y[1]); \
                    z[0]= (tr/td); \
                    z[1]= (ti/td); \
                } \
\
                /* Output final result to matrix c. */ \
                PASTEMAC(ch,copys)( beta11c, *gamma11 ); \
\
                /* Store the local value back to b11. */ \
                PASTEMAC(ch,copys)( beta11c, *beta11 ); \
            } \
        } \
}

INSERT_GENTFUNC_BASIC2( trsm_u, BLIS_CNAME_INFIX, BLIS_REF_SUFFIX )
#endif //BLIS_DISABLE_TRSM_PREINVERSION


#endif// #if 0
