// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cmath>
#include <cstdint>
#include <random>

// #include "GraphBLAS.h"
#include "../GB_Matrix_allocate.h"

static const char *_cudaGetErrorEnum(cudaError_t error) {
  return cudaGetErrorName(error);
}

template <typename T>
void check(T result, char const *const func, const char *const file,
           int const line) {
  if (result) {
    fprintf(stderr, "CUDA error at %s:%d code=%d(%s) \"%s\" \n", file, line,
            static_cast<unsigned int>(result), _cudaGetErrorEnum(result), func);
    exit(EXIT_FAILURE);
  }
}

#define checkCudaErrors(val) check((val), #val, __FILE__, __LINE__)

// This will output the proper error string when calling cudaGetLastError
#define getLastCudaError(msg) __getLastCudaError(msg, __FILE__, __LINE__)

inline void __getLastCudaError(const char *errorMessage, const char *file,
                               const int line) {
  cudaError_t err = cudaGetLastError();

  if (cudaSuccess != err) {
    fprintf(stderr,
            "%s(%i) : getLastCudaError() CUDA error :"
            " %s : (%d) %s.\n",
            file, line, errorMessage, static_cast<int>(err),
            cudaGetErrorString(err));
    exit(EXIT_FAILURE);
  }
}

// This will only print the proper error string when calling cudaGetLastError
// but not exit program incase error detected.
#define printLastCudaError(msg) __printLastCudaError(msg, __FILE__, __LINE__)

inline void __printLastCudaError(const char *errorMessage, const char *file,
                                 const int line) {
  cudaError_t err = cudaGetLastError();

  if (cudaSuccess != err) {
    fprintf(stderr,
            "%s(%i) : getLastCudaError() CUDA error :"
            " %s : (%d) %s.\n",
            file, line, errorMessage, static_cast<int>(err),
            cudaGetErrorString(err));
  }
}
#define CHECK_CUDA(call) checkCudaErrors( call )

//Vector generators
template<typename T>
void fillvector_linear( int N, T *vec) {
   for (int i = 0; i< N; ++i) vec[i] = T(i);
}
template<typename T>
void fillvector_constant( int N, T *vec, T val) {
   for (int i = 0; i< N; ++i) vec[i] = val;
}

// Mix-in class to enable unified memory
class Managed {
public:
  void *operator new(size_t len) {
    void *ptr = nullptr;
    //std::cout<<"in new operator, alloc for "<<len<<" bytes"<<std::endl;
    CHECK_CUDA( cudaMallocManaged( &ptr, len) );
    cudaDeviceSynchronize();
    //std::cout<<"in new operator, sync "<<len<<" bytes"<<std::endl;
    return ptr;
  }

  void operator delete(void *ptr) {
    cudaDeviceSynchronize();
    //std::cout<<"in delete operator, free "<<std::endl;
    CHECK_CUDA( cudaFree(ptr) );
  }
};

//Basic matrix container class
template<typename T>
class matrix : public Managed {
  public:
    uint64_t zombie_count = 0;
    GrB_Matrix mat;
     int64_t vlen;
     int64_t vdim;
     int64_t nnz;
//     int64_t *p = nullptr;
//     int64_t *h = nullptr;
//     int64_t *i = nullptr;
//     T *x = nullptr;
     bool is_filled = false;

//     // TODO: Need to figure out whether this container is needed or whether it should wrap the GB_Matrix
//     int64_t nzmax;
//    int64_t nvals;

     matrix(){
     };

//     matrix( int64_t N, int64_t nvecs) {
//        vlen = N;
//        vdim = nvecs;
//     }

     GrB_Matrix get_grb_matrix() {
         return mat;
     }

     void set_zombie_count( uint64_t zc) { mat->nzombies = zc;}
     uint64_t get_zombie_count() { return mat->nzombies;}
     void add_zombie_count( int nz) { mat->nzombies += nz;}

     void clear() {
//        if ( mat->p != nullptr){  cudaFree(mat->p); mat->p = nullptr; }
//        if ( mat->h != nullptr){  cudaFree(mat->h); mat->h = nullptr; }
//        if ( mat->i != nullptr){  cudaFree(mat->i); mat->i = nullptr; }
//        if ( mat->x != nullptr){  cudaFree(mat->x); mat->x = nullptr; }
//        is_filled = false;
//         nnz  = 0;
//        mat->vlen = 0;
//        mat->vdim = 0;
//        mat->nzombies = 0;
     }

     void alloc( int64_t N, int64_t Nz) {
         // allocate a GrB_FP32 matrix (but the A->type is NULL;
         // the GPU kernel cannot access it anyway since GrB_FP32
         // is a pointer to a global, statically declared struct on the CPU.
         mat = GB_matrix_allocate(NULL, sizeof(float), N, N, 2, false, false, Nz, -1);
     }
 
     void fill_random(  int64_t N, int64_t Nz, std::mt19937 r) {

         std::cout << "inside fill" << std::endl;
         alloc( N, Nz);
         int64_t *p = mat->p;
         int64_t *i = mat->i;
         T *x = (T*)mat->x;
        int64_t inv_sparsity = (N*N)/Nz;   //= values not taken per value occupied in index space

        std::cout<< "fill_random N="<< N<<" need "<< Nz<<" values, invsparse = "<<inv_sparsity<<std::endl;

        std::cout<< "fill_random"<<" after alloc values"<<std::endl;
        mat->vdim = N;
        std::cout<<"vdim ready "<<std::endl;
        mat->vlen = N;
        std::cout<<"vlen ready "<<std::endl;
        nnz = Nz;
        std::cout<<"ready to fill p"<<std::endl;

        mat->p[0] = 0;
        mat->p[N] = nnz;

        std::cout<<"   in fill loop"<<std::endl;
        for (int64_t j = 0; j < N; ++j) {
           p[j+1] = p[j] + Nz/N;
           for ( int k = p[j] ; k < p[j+1]; ++k) {
               i[k] = (k-p[j])*inv_sparsity +  r() % inv_sparsity;
               x[k] = (T) (k & 63) ;
           }
        }
        is_filled = true;
     }
};



template< typename T_C, typename T_M, typename T_A, typename T_B>
class SpGEMM_problem_generator {

    float Anzpercent,Bnzpercent,Cnzpercent;
    int64_t Cnz;
    int64_t *Bucket = nullptr;

    int64_t BucketStart[13];
    unsigned seed = 13372801;
    std::mt19937 r; //random number generator Mersenne Twister
    bool ready = false;

  public:

    matrix<T_C> *C= nullptr;
    matrix<T_M> *M= nullptr;
    matrix<T_A> *A= nullptr;
    matrix<T_B> *B= nullptr;

    SpGEMM_problem_generator() {
    
       //std::cout<<"creating matrices"<<std::endl;
       // Create sparse matrices
       C = new matrix<T_C>;
       // CHECK_CUDA( cudaMallocManaged( (void**)&C, sizeof(matrix<T_C>)) );
       //cudaMemAdvise ( C, sizeof(matrix<T_C>), cudaMemAdviseSetReadMostly, 1);
       //std::cout<<"created  C matrix"<<std::endl;
       M = new matrix<T_M>;
       //cudaMallocManaged( (void**)&M, sizeof(matrix<T_M>));
       //cudaMemAdvise ( M, sizeof(matrix<T_C>), cudaMemAdviseSetReadOnly, 1);
       //std::cout<<"created  M matrix"<<std::endl;
       A = new matrix<T_A>;
       //cudaMallocManaged( (void**)&A, sizeof(matrix<T_A>));
       //cudaMemAdvise ( C, sizeof(matrix<T_C>), cudaMemAdviseSetReadOnly, 1);
       //std::cout<<"created  A matrix"<<std::endl;
       B = new matrix<T_B>;
       //cudaMallocManaged( (void**)&B, sizeof(matrix<T_B>));
       //cudaMemAdvise ( C, sizeof(matrix<T_C>), cudaMemAdviseSetReadOnly, 1);
       //std::cout<<"created  B matrix"<<std::endl;

    };

    matrix<T_C>* getCptr(){ return C;}
    matrix<T_M>* getMptr(){ return M;}
    matrix<T_A>* getAptr(){ return A;}
    matrix<T_B>* getBptr(){ return B;}

    int64_t* getBucket() { return Bucket;}
    int64_t* getBucketStart(){ return BucketStart;}

    void loadCj() {

       // Load C_i with column j info to avoid another lookup
       for (int c = 0 ; c< M->mat->vdim; ++c) {
           for ( int r = M->mat->p[c]; r< M->mat->p[c+1]; ++r){
               C->mat->i[r] = c << 4 ; //shift to store bucket info
           }
       }

    }

    void init( int64_t N , int64_t Anz, int64_t Bnz, float Cnzpercent){

       // Get sizes relative to fully dense matrices
       Anzpercent = float(Anz)/float(N*N);
       Bnzpercent = float(Bnz)/float(N*N);
       Cnzpercent = Cnzpercent;
       Cnz = (int64_t)(Cnzpercent * N * N);
       std::cout<<"Anz% ="<<Anzpercent<<" Bnz% ="<<Bnzpercent<<" Cnz% ="<<Cnzpercent<<std::endl;

       //Seed the generator
       r.seed(seed);

       std::cout<<"filling matrices"<<std::endl;

       C->fill_random( N, Cnz, r);
       M->fill_random( N, Cnz, r);
       A->fill_random( N, Anz, r);
       B->fill_random( N, Bnz, r);


       std::cout<<"fill complete"<<std::endl;
       C->mat->p = M->mat->p; //same column pointers (assuming CSC here)

       loadCj();

    }

    void del(){
       C->clear();
       M->clear();
       A->clear();
       B->clear();
       if (Bucket != nullptr) CHECK_CUDA( cudaFree(Bucket) );
       delete C;
       delete M;
       delete A;
       delete B;
       CHECK_CUDA( cudaDeviceSynchronize() );
    }

    void fill_buckets( int fill_bucket){

       std::cout<<Cnz<<" slots to fill"<<std::endl;

       if (fill_bucket == -1){  

       // Allocate Bucket space
       CHECK_CUDA( cudaMallocManaged((void**)&Bucket, Cnz*sizeof(int64_t)) );

       //Fill buckets with random extents such that they sum to Cnz, set BucketStart
           BucketStart[0] = 0; 
           BucketStart[12] = Cnz;
           for (int b = 1; b < 12; ++b){
              BucketStart[b] = BucketStart[b-1] + (Cnz / 12);  
              //std::cout<< "bucket "<< b<<" starts at "<<BucketStart[b]<<std::endl;
              for (int j = BucketStart[b-1]; j < BucketStart[b]; ++j) { 
                Bucket[j] = b ; 
              }
           }
           int b = 11;
           for (int j = BucketStart[11]; j < BucketStart[12]; ++j) { 
                Bucket[j] = b ; 
           }
       }
       else {// all in one test bucket
           Bucket = nullptr;
           BucketStart[0] = 0; 
           BucketStart[12] = Cnz;
           for (int b= 0; b<12; ++b){
              if (b <= fill_bucket) BucketStart[b] = 0;
              if (b  > fill_bucket) BucketStart[b] = Cnz;
              //std::cout<< " one  bucket "<< b<<"starts at "<<BucketStart[b]<<std::endl;
           } 
           std::cout<<"all pairs to bucket "<<fill_bucket<<", no filling"<<std::endl;
           std::cout<<"done assigning buckets"<<std::endl;
       }
    }
};


