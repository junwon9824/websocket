
// base64.c
#include <stdio.h>
#include <string.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h> // BUF_MEM 정의를 포함


void base64_encode(const unsigned char* input, int length, char* output) {
    BIO* bio;
    BIO* b64;
    BUF_MEM* bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // 줄바꿈 없이 인코딩

    BIO_write(bio, input, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);
    BIO_set_close(bio, BIO_NOCLOSE);
    BIO_free_all(bio);

    // 출력 버퍼에 복사
    if (bufferPtr->length > 0) {
        memcpy(output, bufferPtr->data, bufferPtr->length);
        output[bufferPtr->length] = '\0'; // null-terminate
    }
}
