#include "TrieBuffer.hpp"
#include "Trie.hpp"
#include "Vector.hpp"
#include "VectorOps.hpp"
#include "load.hpp"

int main()
{
 thread_pool::initializeThreadPool();

  const size_t mat_size = 16;
  auto tup = load_dense_matrix_and_transpose(mat_size,mat_size);

  //auto tup = load_matrix_and_transpose("../../../matrix_benchmarking/data/simple.tsv");
  Trie<float,ParMemoryBuffer> *M = tup.first;
  Trie<float,ParMemoryBuffer> *M_T = tup.second;

  auto query_time = timer::start_clock();
  const size_t num_nprr_cols = 4;
  Trie<float,ParMemoryBuffer>* result = new Trie<float,ParMemoryBuffer>("",4,true);
  
  //temporary buffers for aggregate intersections.
  ParMemoryBuffer **tmp_buffers = new ParMemoryBuffer*[num_nprr_cols];
  for(size_t i = 0; i < num_nprr_cols; i++){
    tmp_buffers[i] = new ParMemoryBuffer(100);
  }

  //M(a,b),M_T(c,b)
  Vector<DenseVector,BufferIndex,ParMemoryBuffer> M_head(
    M->memoryBuffers);
  Vector<DenseVector,BufferIndex,ParMemoryBuffer> M_T_head(
    M_T->memoryBuffers);

  Vector<DenseVector,BufferIndex,ParMemoryBuffer> I = 
    Vector<DenseVector,BufferIndex,ParMemoryBuffer>(
      NUM_THREADS,
      result->memoryBuffers,
      M_head);

  /*
  std::cout << "HERE" << std::endl;
  M->print();
  std::cout << "TRANSPOSE" << std::endl;
  M_T->print();
  */

  //allocate a dense square for i and j
  //block size for i and block size * block size for j
  TrieBuffer<float,ParMemoryBuffer>* tmp_block = 
    new TrieBuffer<float,ParMemoryBuffer>(2,true);

  I.parforeach_index([&](const size_t tid, const uint32_t I_i, const uint32_t I_d){
    (void) I_i;
    BufferIndex I_nl = M_head.get(I_d);
    Vector<DenseVector,BufferIndex,ParMemoryBuffer> K_I(
      M->memoryBuffers,
      I_nl);
    Vector<DenseVector,BufferIndex,ParMemoryBuffer> J = 
      Vector<DenseVector,BufferIndex,ParMemoryBuffer>(
        tid,
        result->memoryBuffers,
        M_T_head.get_this(),
        M_T_head.get_num_index_bytes(),
        M_T_head.get_num_annotation_bytes<BufferIndex>());
    J.foreach_index([&](const uint32_t J_i, const uint32_t J_d){
      (void)J_i;
      BufferIndex J_nl = M_T_head.get(J_d);
      Vector<DenseVector,BufferIndex,ParMemoryBuffer> K_J(
        M_T->memoryBuffers,
        J_nl);
      Vector<DenseVector,void*,ParMemoryBuffer> K = 
        ops::agg_intersect<BufferIndex,BufferIndex>(
          tid,
          tmp_buffers[0],
          K_I,
          K_J);

      std::vector<size_t> block_offsets;
      block_offsets.push_back(I_d*BLOCK_SIZE);
      block_offsets.push_back(J_d*BLOCK_SIZE);
      tmp_block->zero(block_offsets); //zero out the memory.

      K.foreach_index([&](const uint32_t K_i, const uint32_t K_d){      
        (void)K_i;
        BufferIndex M_K_nl = K_I.get(K_d);
        BufferIndex M_T_K_nl = K_J.get(K_d);
        
        Vector<DenseVector,BufferIndex,ParMemoryBuffer> i_K(
          M->memoryBuffers,
          M_K_nl);

        Vector<DenseVector,BufferIndex,ParMemoryBuffer> j_K(
          M_T->memoryBuffers,
          M_T_K_nl);

        //tmp_block(0) union i_K 
        Vector<DenseVector,void*,ParMemoryBuffer> i = 
          ops::union_in_place(tmp_block->at(0,0),i_K);

        i.foreach_index([&](const uint32_t i_i, const uint32_t i_d){
          BufferIndex k_i_nl = i_K.get(i_d);
          Vector<DenseVector,float,ParMemoryBuffer> k_i(
            M->memoryBuffers,
            k_i_nl);

          Vector<DenseVector,void*,ParMemoryBuffer> j = 
            ops::union_in_place(tmp_block->at(1,i_d),j_K);

          j_K.foreach_index([&](const uint32_t j_i, const uint32_t j_d){
            BufferIndex k_j_nl = j_K.get(j_d);
            Vector<DenseVector,float,ParMemoryBuffer> k_j(
              M_T->memoryBuffers,
              k_j_nl);
            float anno = ops::agg_intersect(
              tid,
              tmp_buffers[1],
              k_j,
              k_i);
            //std::cout << I_d << " " << J_d << " " << i_d << " " << j_d << " " << anno << std::endl;
            tmp_block->get_anno(i_d%BLOCK_SIZE)[j_d%BLOCK_SIZE] += anno;
          });
        });

        //copy buffer into trie.
        //J.set(J_i,J_d,i.bufferIndex);
      });
      tmp_block->print(I_d,J_d);
    });
    I.set(I_i,I_d,J.bufferIndex);
  });

  timer::stop_clock("QUERY",query_time);


/*
  size_t num_output = 0;
  const size_t max_num_output = mat_size*mat_size;
  Encoding<uint32_t> *enc = (Encoding<uint32_t>*)M->encodings.at(0);
  result->foreach([&](std::vector<uint32_t> *v,float anno){
    if(anno != 0){
      for(size_t i = 0; i < v->size(); i++){
        std::cout << enc->key_to_value.at(v->at(i)) << "\t";
      }
      std::cout << anno << std::endl;
      num_output++;
    }
    if(num_output > max_num_output)
      exit(0);
  });
*/

  return 0;
}