#pragma warning(disable: 4101)



#include <stdlib.h>
#ifdef _WIN32
# include <crtdbg.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <signal.h>

#include <argparse.h>

#include <fileu.h>



#include <sorcer.h>
#include <sorcer_utils.h>





static int mainReturn(int r)
{
#if !defined(NDEBUG) && defined(_WIN32)
    system("pause");
#endif
    return r;
}









static void execCode(const char* filename, const char* code)
{
    //char timeBuf[SORCER_TimeStrBuf_MAX];
    //printf("[EXEC] \"%s\" [%s]\n", filename, TXN_evalGetNowStr(timeBuf));
    //SORCER_Context* ctx = SORCER_ctxNew(NULL);
    //bool r;
    //if (code)
    //{
    //    r = SORCER_execCode(ctx, filename, code, true);
    //}
    //else
    //{
    //    r = SORCER_execFile(ctx, filename, true);
    //}
    //SORCER_Error err = SORCER_ctxLastError(ctx);
    //if (r)
    //{
    //    assert(SORCER_ErrCode_NONE == err.code);
    //    printf("[DONE] \"%s\" [%s]\n", filename, TXN_evalGetNowStr(timeBuf));
    //}
    //else
    //{
    //    const SORCER_FileInfoTable* fiTable = SORCER_ctxSrcFileInfoTable(ctx);
    //    TXN_evalErrorFprint(stderr, fiTable, &err);
    //}
    //if (r)
    //{
    //    printf("<DataStack>\n");
    //    printf("-------------\n");
    //    TXN_evalDataStackFprint(stdout, ctx);
    //    printf("-------------\n");
    //}
    //SORCER_ctxFree(ctx);
}




static bool quitFlag = false;
void intHandler(int dummy)
{
    quitFlag = true;
}






int main(int argc, char* argv[])
{
#if !defined(NDEBUG) && defined(_WIN32)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    char timeBuf[SORCER_TimeStrBuf_MAX];

    char* entryFile = NULL;
    int watchFlag = false;
    struct argparse_option options[] =
    {
        OPT_HELP(),
        //OPT_GROUP("Basic options"),
        OPT_STRING('f', "file", &entryFile, "execute entry file"),
        OPT_BOOLEAN('w', "watch", &watchFlag, "watch file and execute it when it changes"),
        OPT_END(),
    };
    struct argparse argparse;
    argparse_init(&argparse, options, NULL, 0);
    argc = argparse_parse(&argparse, argc, argv);

    if (entryFile)
    {
        time_t lastMtime;
        struct stat st;
        {
            stat(entryFile, &st);
            lastMtime = st.st_mtime;
        }
        execCode(entryFile, NULL);
        if (watchFlag)
        {
            signal(SIGINT, intHandler);
            while (!quitFlag)
            {
                stat(entryFile, &st);
                if (lastMtime != st.st_mtime)
                {
                    printf("[CHANGE] \"%s\" [%s]\n", entryFile, SORCER_nowStr(timeBuf));
                    //execCode(entryFile, NULL);
                }
                lastMtime = st.st_mtime;
            }
        }
    }
    else
    {
        vec_char code = { 0 };
        for (;;)
        {
            char c = getchar();
            if (EOF == c) break;
            vec_push(&code, c);
        }
        vec_push(&code, 0);
        execCode("stdin", code.data);
        vec_free(&code);
    }

    return mainReturn(EXIT_SUCCESS);
}






























































