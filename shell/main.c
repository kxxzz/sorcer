#pragma warning(disable: 4101)



#include <stdlib.h>
#ifdef _WIN32
# include <crtdbg.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <sys/stat.h>
#include <signal.h>

#include <argparse.h>

#include <fileu.h>



#include <sorcer.h>
#include <sorcer_mana.h>
#include <sorcer_arith.h>
#include <sorcer_utils.h>





static int mainReturn(int r)
{
#if !defined(NDEBUG) && defined(_WIN32)
    system("pause");
#endif
    return r;
}






enum
{
    TimeStrBuf_MAX = 16,
};

static char* nowStr(char* timeBuf)
{
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    timeBuf[strftime(timeBuf, TimeStrBuf_MAX, "%H:%M:%S", lt)] = '\0';
    return timeBuf;
}




static void execCode(const char* filename, const char* code)
{
    char timeBuf[TimeStrBuf_MAX];
    printf("[COMP START] \"%s\" [%s]\n", filename, nowStr(timeBuf));
    SORCER_Context* ctx = SORCER_ctxNew();
    SORCER_ArithContext arithCtx[1] = { 0 };
    SORCER_arith(ctx, arithCtx);
    SORCER_ManaErrorInfo errInfo[1] = { 0 };
    SORCER_ManaFileInfoVec fileTable[1] = { 0 };
    SORCER_Block blk;
    if (code)
    {
        blk = SORCER_blockFromManaStr(ctx, code, errInfo, fileTable);
    }
    else
    {
        blk = SORCER_blockFromManaFile(ctx, filename, errInfo, fileTable);
    }
    printf("[COMP DONE] \"%s\" [%s]\n", filename, nowStr(timeBuf));
    if (blk.id != SORCER_Block_Invalid.id)
    {
        assert(SORCER_ManaError_NONE == errInfo->error);

        SORCER_RunErrorInfo runErr[1];

        printf("[EXEC START] \"%s\" [%s]\n", filename, nowStr(timeBuf));
        SORCER_run(ctx, blk, runErr);
        printf("[EXEC DONE] \"%s\" [%s]\n", filename, nowStr(timeBuf));

        if (runErr->error)
        {
            const char* errname = SORCER_RunErrorNameTable(runErr->error);
            const char* filename = fileTable->data[runErr->file].path;
            printf
            (
                "[EXEC ERROR] %s: \"%s\": %u:%u [%s]\n",
                errname, filename, runErr->line, runErr->column, nowStr(timeBuf)
            );
        }
        else
        {
            printf("<DataStack>\n");
            printf("-------------\n");
            SORCER_dsFprint(stdout, ctx);
            printf("-------------\n");
        }
    }
    else
    {
        const char* errname = SORCER_ManaErrorNameTable(errInfo->error);
        const char* filename = fileTable->data[errInfo->file].path;
        printf
        (
            "[COMP ERROR] %s: \"%s\": %u:%u [%s]\n",
            errname, filename, errInfo->line, errInfo->column, nowStr(timeBuf)
        );
    }
    vec_free(fileTable);
    SORCER_ctxFree(ctx);
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

    char timeBuf[TimeStrBuf_MAX];

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
                    printf("[CHANGE] \"%s\" [%s]\n", entryFile, nowStr(timeBuf));
                    execCode(entryFile, NULL);
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






























































