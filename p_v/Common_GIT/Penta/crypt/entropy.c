#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if WIN32
#include <windows.h>
#include <wincrypt.h>
#include <tlhelp32.h>
//typedef BOOL (WINAPI *CRYPTACQUIRECONTEXT)(HCRYPTPROV *, LPCWSTR, LPCWSTR, DWORD, DWORD);
//typedef BOOL (WINAPI *CRYPTGENRANDOM)(HCRYPTPROV, DWORD, BYTE *);
//typedef BOOL (WINAPI *CRYPTRELEASECONTEXT)(HCRYPTPROV, DWORD);
typedef BOOL (WINAPI *CRYPTACQUIRECONTEXT)(HCRYPTPROV *, LPCWSTR, LPCWSTR, UINT, UINT);
typedef BOOL (WINAPI *CRYPTGENRANDOM)(HCRYPTPROV, UINT, BYTE *);
typedef BOOL (WINAPI *CRYPTRELEASECONTEXT)(HCRYPTPROV, UINT);
#else
#ifndef DUKPT_CLIENT_NOT_OS
//#include <sys/time.h>
//#include <unistd.h>
//#include <sys/stat.h>
//#include <sys/utsname.h>
#endif
#endif
#include "sha256.h"
#include "entropy.h"

#ifdef DUKPT_CLIENT_NOT_OS
#define ENTROPY_DATA_MAX 512
#else
#define ENTROPY_DATA_MAX 4096
#endif
#define ENTROPY_USE 256

extern uint32_t GetLocalTimefromTime(uint32_t unUTCTime);
extern uint32_t GetUTCTime();

static int DAMO_CRYPT_Get_Entropy_Source(unsigned char *entropy_data, int *out_entropy_data_len)
{
  int entropy_data_len = 0;
#ifndef WIN32
  unsigned long l;
  //int len;
#ifndef DUKPT_CLIENT_NOT_OS
  struct timeval tv;
  struct stat sfs;
  struct utsname un;
#endif
#ifndef DUKPT_NOT_IO
  FILE *fp;
#endif
#endif

#if WIN32
  HCRYPTPROV hProvider = 0;
  HMODULE advapi = LoadLibrary(TEXT("ADVAPI32.DLL"));
  CRYPTACQUIRECONTEXT acquire = NULL;
  CRYPTGENRANDOM gen = NULL;
  CRYPTRELEASECONTEXT release = NULL;
  unsigned int tmpbuf[256];

  if (advapi)
  {
    acquire = (CRYPTACQUIRECONTEXT) GetProcAddress(advapi, "CryptAcquireContextA");
    gen = (CRYPTGENRANDOM) GetProcAddress(advapi, "CryptGenRandom");
    release = (CRYPTRELEASECONTEXT) GetProcAddress(advapi, "CryptReleaseContext");
  }

  if (acquire && gen && release)
  {
    if (acquire(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
      if (gen(hProvider, sizeof(tmpbuf), (unsigned char *)tmpbuf) != 0)
      {
        memcpy(entropy_data+entropy_data_len, tmpbuf, sizeof(tmpbuf));
        entropy_data_len += sizeof(tmpbuf);
      }
      release(hProvider, 0);
    }
  }
  if (advapi)
    FreeLibrary(advapi);

#else
#ifndef DUKPT_NOT_IO
  if ((fp = fopen("/dev/urandom", "r")) != NULL)
  {
    len=fread((unsigned char *) entropy_data, 1, ENTROPY_USE, fp);
    fclose(fp);

    entropy_data_len += ENTROPY_USE;
  }
#else
  /*need random value to fill entropy_data */ 
  // 180323 SPARROW
  entropy_data_len += ENTROPY_USE;
#endif
  
  // 180323 SPARROW
  // time 함수 사용할 수 없음. 아래 함수 호출하면서 단말 재부팅 해버림 => Time 가져오는 함수를 변경함
  //l=time(NULL);
  l=GetLocalTimefromTime(GetUTCTime());
  memcpy(entropy_data+entropy_data_len, &l, sizeof(l));
  entropy_data_len += sizeof(l);

#ifndef DUKPT_CLIENT_NOT_OS
  l=getpid();
  memcpy(entropy_data+entropy_data_len, &l, sizeof(l));
  entropy_data_len += sizeof(l);

  l=getuid();
  memcpy(entropy_data+entropy_data_len, &l, sizeof(l));
  entropy_data_len += sizeof(l);

  l=getgid();
  memcpy(entropy_data+entropy_data_len, &l, sizeof(l));
  entropy_data_len += sizeof(l);

  gettimeofday(&tv, NULL);
  l=tv.tv_usec;
  memcpy(entropy_data+entropy_data_len, &l, sizeof(l));
  entropy_data_len += sizeof(l);

  stat("/", &sfs);
  memcpy(entropy_data+entropy_data_len, &sfs, sizeof(sfs));
  entropy_data_len += sizeof(sfs);

  gethostname((char *)entropy_data+entropy_data_len, 32);
  entropy_data_len += 32;

  uname(&un);
  memcpy(entropy_data+entropy_data_len, &un, sizeof(un));
  entropy_data_len += sizeof(un);
#endif
#endif

  *out_entropy_data_len = entropy_data_len;

  return DAMO_CRYPT_SUCCESS;
}

int DAMO_CRYPT_Get_SHA256_Entropy_Data(void *data, unsigned char *output, size_t output_len)
{
  int i;
  unsigned char digest[32];
  size_t digest_len;
  size_t len;
  unsigned char entropy_data[ENTROPY_DATA_MAX]={0};
  int entropy_data_len=0;

  DAMO_CRYPT_Get_Entropy_Source(entropy_data, &entropy_data_len);

  len = 0;

  while(len < output_len)
  {
    DAMO_CRYPT_HASH_SHA256(entropy_data, entropy_data_len, digest);
    digest_len = 32;

    if((output_len - len) < digest_len) digest_len = (output_len - len);

    memcpy(output + len, digest, digest_len);

    len += digest_len;

    if(len < output_len)
    {
      for(i = 0; i < 80; ++i)
      {
        entropy_data[i] ^= entropy_data[80-i];
      }
    }
  }

  memset(entropy_data, 0, entropy_data_len);

  return DAMO_CRYPT_SUCCESS;
}


