#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "max_values.h"

#include "main_decryption.h"
#include "main_key_ceremony.h"
#include "main_params.h"
#include "main_voting.h"

static FILE *fmkstemps(char const *template, int suffixlen, const char *mode);

// Parameters
uint32_t const NUM_TRUSTEES = 10;
uint32_t const THRESHOLD = 8;
uint32_t const NUM_ENCRYPTERS = 3;
uint32_t const NUM_SELECTIONS = 3;

int main()
{
    srand(100);

    bool ok = true;

    // Outputs of the key ceremony
    struct trustee_state trustee_states[MAX_TRUSTEES];
    struct joint_public_key joint_key;

    // Key Ceremony
    if (ok)
        ok = key_ceremony(&joint_key, trustee_states);

    // Open the voting results files
    FILE *voting_results = NULL;

    if (ok)
    {
        voting_results = fmkstemps("voting_results-XXXXXX.txt", 4, "w");
        if (voting_results == NULL)
            ok = false;
    }

    // Voting

    if (ok)
        ok = voting(joint_key, voting_results);

    // Rewind the voting results file to the beginning and set mode to reading
    if (ok)
    {
        voting_results = freopen(NULL, "r", voting_results);
        if (voting_results == NULL)
            ok = false;
    }

    // Open the tally file
    FILE *tally = NULL;

    if (ok)
    {
        tally = fmkstemps("tally-XXXXXX.txt", 4, "w");
        if (tally == NULL)
            ok = false;
    }

    // Decryption
    if (ok)
        ok = decryption(voting_results, tally, trustee_states);

    // Cleanup
    if (voting_results != NULL)
    {
        fclose(voting_results);
        voting_results = NULL;
    }

    if (tally != NULL)
    {
        fclose(tally);
        tally = NULL;
    }

    for (uint32_t i = 0; i < NUM_TRUSTEES; i++)
        if (trustee_states[i].bytes != NULL)
        {
            free((void *)trustee_states[i].bytes);
            trustee_states[i].bytes = NULL;
        }

    if (joint_key.bytes != NULL)
    {
        free((void *)joint_key.bytes);
        joint_key.bytes = NULL;
    }

    if (ok)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}

FILE *fmkstemps(char const *template, int suffixlen, const char *mode)
{
    bool ok = true;

    FILE *result = NULL;

    char *template_mut = strdup(template);
    if (template_mut == NULL)
        ok = false;

    int fd = -1;
    if (ok)
    {
        fd = mkstemps(template_mut, suffixlen);
        if (fd < 0)
            ok = false;
    }

    if (ok)
    {
        result = fdopen(fd, mode);
        if (result == NULL)
            ok = false;
    }

    if (!ok && fd >= 0)
    {
        close(fd);
        fd = -1;
    }

    if (template_mut != NULL)
    {
        free(template_mut);
        template_mut = NULL;
    }

    return result;
}
