/***************************************************************************
 * OpenSSL support for Objeck
 *
 * Copyright (c) 2011-2013, Randy Hollines
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright * notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the distribution.
 * - Neither the name of the Objeck Team nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

#include <string.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/md5.h>
#include <openssl/ripemd.h>
#include "../../../vm/lib_api.h"

using namespace std;

extern "C" {
  //
  // initialize library
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void load_lib() {
  }
  
  //
  // release library
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void unload_lib() {
  }
  
  //
  // SHA-256 hash
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void openssl_hash_sha256(VMContext& context) {
    // get parameters
    long* input_array = (long*)APITools_GetIntAddress(context, 1)[0];    
    int input_size =  APITools_GetArraySize(input_array) - 1;
    const unsigned char* input =  (unsigned char*)APITools_GetByteArray(input_array);
    
    // hash 
    unsigned char output[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input, input_size);
    SHA256_Final(output, &sha256);

    // copy output
    long* output_byte_array = APITools_MakeByteArray(context, SHA256_DIGEST_LENGTH);
    unsigned char* output_byte_array_buffer = (unsigned char*)(output_byte_array + 3);
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
      output_byte_array_buffer[i] = output[i];
    }
    
    long* output_holder = APITools_GetIntAddress(context, 0);
    output_holder[0] = (long)output_byte_array;   
  }
  
  //
  // RIPEMD-160 hash
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void openssl_hash_ripemd160(VMContext& context) {
    // get parameters
    long* input_array = (long*)APITools_GetIntAddress(context, 1)[0];    
    int input_size =  APITools_GetArraySize(input_array) - 1;
    const unsigned char* input =  (unsigned char*)APITools_GetByteArray(input_array);
    
    // hash 
    unsigned char output[RIPEMD160_DIGEST_LENGTH];
    RIPEMD160_CTX sha256;
    RIPEMD160_Init(&sha256);
    RIPEMD160_Update(&sha256, input, input_size);
    RIPEMD160_Final(output, &sha256);

    // copy output
    long* output_byte_array = APITools_MakeByteArray(context, RIPEMD160_DIGEST_LENGTH);
    unsigned char* output_byte_array_buffer = (unsigned char*)(output_byte_array + 3);
    for(int i = 0; i < RIPEMD160_DIGEST_LENGTH; i++) {
      output_byte_array_buffer[i] = output[i];
    }
    
    long* output_holder = APITools_GetIntAddress(context, 0);
    output_holder[0] = (long)output_byte_array;   
  }
  
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void openssl_hash_md5(VMContext& context) {
    // get parameters
    long* input_array = (long*)APITools_GetIntAddress(context, 1)[0];    
    int input_size =  APITools_GetArraySize(input_array) - 1;
    const unsigned char* input =  (unsigned char*)APITools_GetByteArray(input_array);
    
    // hash 
    unsigned char output[MD5_DIGEST_LENGTH];
    MD5(input, input_size, output);
    
    // copy output
    long* output_byte_array = APITools_MakeByteArray(context, MD5_DIGEST_LENGTH);
    unsigned char* output_byte_array_buffer = (unsigned char*)(output_byte_array + 3);
    for(int i = 0; i < MD5_DIGEST_LENGTH; i++) {
      output_byte_array_buffer[i] = output[i];
    }
    
    long* output_holder = APITools_GetIntAddress(context, 0);
    output_holder[0] = (long)output_byte_array;   
  }

  //
  // AES-256 encryption
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void openssl_encrypt_aes256(VMContext& context) {
    // get parameters
    long* key_array = (long*)APITools_GetIntAddress(context, 1)[0];    
    const int key_size =  APITools_GetArraySize(key_array) - 1;
    const unsigned char* key =  (unsigned char*)APITools_GetByteArray(key_array);
    
    long* input_array = (long*)APITools_GetIntAddress(context, 2)[0];    
    const int input_size =  APITools_GetArraySize(input_array) - 1;
    const unsigned char* input =  (unsigned char*)APITools_GetByteArray(input_array);
    
    // TODO: add salt
    unsigned char* salt = NULL;
    
    // encrypt
    EVP_CIPHER_CTX ctx;
    unsigned char iv[32];
    unsigned char key_out[32];
    int result = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key, key_size, 8, key_out, iv);
    if (result != 32) {
      EVP_CIPHER_CTX_cleanup(&ctx);
      return;
    }
    
    int output_size = input_size + AES_BLOCK_SIZE;     
    unsigned char* output = (unsigned char*)calloc(output_size, sizeof(unsigned char));    
    EVP_CIPHER_CTX_init(&ctx);
    if(!EVP_EncryptInit_ex(&ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
      EVP_CIPHER_CTX_cleanup(&ctx);      
      free(output);
      output = NULL;
      return;
    }
    
    if(!EVP_EncryptUpdate(&ctx, output, &output_size, input, input_size)) {
      EVP_CIPHER_CTX_cleanup(&ctx);      
      free(output);
      output = NULL;
      return;
    }
    
    int final_size;
    if(!EVP_EncryptFinal_ex(&ctx, output + output_size, &final_size)) {
      EVP_CIPHER_CTX_cleanup(&ctx);      
      free(output);
      output = NULL;
      return;
    }
    
    EVP_CIPHER_CTX_cleanup(&ctx);
    
    // copy output
    const int total_size = output_size + final_size;
    long* output_byte_array = APITools_MakeByteArray(context, total_size);
    unsigned char* output_byte_array_buffer = (unsigned char*)(output_byte_array + 3);
    for(int i = 0; i < total_size; i++) {
      output_byte_array_buffer[i] = output[i];
    }
    free(output);
    output = NULL;
    
    long* output_holder = APITools_GetIntAddress(context, 0);
    output_holder[0] = (long)output_byte_array;    
  }
  
  //
  // AES-256 decryption
  //
#ifdef _WIN32
  __declspec(dllexport) 
#endif
  void openssl_decrypt_aes256(VMContext& context) {
    // get parameters
    long* key_array = (long*)APITools_GetIntAddress(context, 1)[0];    
    const int key_size =  APITools_GetArraySize(key_array) - 1;
    const unsigned char* key =  (unsigned char*)APITools_GetByteArray(key_array);
    
    long* input_array = (long*)APITools_GetIntAddress(context, 2)[0];    
    const int input_size =  APITools_GetArraySize(input_array) - 1;
    const unsigned char* input =  (unsigned char*)APITools_GetByteArray(input_array);
    
    // TODO: add salt
    unsigned char* salt = NULL;
    
    // decrypt
    EVP_CIPHER_CTX ctx;
    unsigned char iv[32];
    unsigned char key_out[32];
    int result = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key, key_size, 8, key_out, iv);
    if (result != 32) {
      EVP_CIPHER_CTX_cleanup(&ctx);
      return;
    }
    
    EVP_CIPHER_CTX_init(&ctx);
    
    if(!EVP_DecryptInit_ex(&ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
      EVP_CIPHER_CTX_cleanup(&ctx);      
      return;
    }
    
    int output_size = input_size + AES_BLOCK_SIZE;   
    unsigned char* output = (unsigned char*)calloc(output_size, sizeof(unsigned char));
    if(!EVP_DecryptUpdate(&ctx, output, &output_size, input, input_size)) {
      EVP_CIPHER_CTX_cleanup(&ctx);      
      free(output);
      output = NULL;
      return;
    }
  
    int final_size;
    if(!EVP_DecryptFinal_ex(&ctx, output + output_size, &final_size))  {
      EVP_CIPHER_CTX_cleanup(&ctx);      
      free(output);
      output = NULL;
      return;
    }
    
    EVP_CIPHER_CTX_cleanup(&ctx);
    
    // copy output
    const int total_size = output_size + final_size;
    long* output_byte_array = APITools_MakeByteArray(context, total_size);
    unsigned char* output_byte_array_buffer = (unsigned char*)(output_byte_array + 3);
    for(int i = 0; i < total_size; i++) {
      output_byte_array_buffer[i] = output[i];
    }
    free(output);
    output = NULL;
    
    long* output_holder = APITools_GetIntAddress(context, 0);
    output_holder[0] = (long)output_byte_array;
  }
}
