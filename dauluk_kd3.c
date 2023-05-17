/*
1. Darydamas uzduotis naudojuosi tik LD1, LD2, LD3 aprasais,
     sistemos pagalbos sistema (man ir pan.), savo darytomis LD uzduotimis
2. Užduoti darau savarankiškai be treciuju asmenu pagalbos
3. Uzduoti koreguoju naudodamasis vienu kompiuteriu

Daugardas Lukšas SUTINKU
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

int(*argv2_ptr);
int(*argv3_ptr);

int main(int argc, char *argv[])
{
    printf("(C) 2023 D.Lukšas.\n");
    if (argc != 4)
    {
        printf("Reikia nurodyti tris argumentus. Buvo nurodyti %d argumentai.\n", argc - 1);
        printf("Naudojimas: %s <simboliu seka> <simboliu seka> <simboliu seka>\n", argv[0]);
        exit(1);
    }

    printf("--1--\n");                                                       // 1 programa priima 3 argumentus (pirmas biblioteka /lib64/vahk.so.1, antras ir trecias bus toj bibliotekoj GAL esantys kintamieji ar funkcijos)
    printf("Jūsų įvesti argumentai: %s %s %s\n", argv[1], argv[2], argv[3]); // prase juos atspausdint

    struct stat lib_stat;
    if (stat(argv[1], &lib_stat) == -1)
    {
        printf("Klaida nuskaitant %s informacija\n", argv[1]);
        exit(1);
    }

    if (S_ISREG(lib_stat.st_mode))
    { // Jeigu biblioteka yra reguliarus faikas, tai atspausdinti jo i-node,
        printf("--2--\n");
        printf("%s - failas. Jo i-node: %ld\n", argv[1], lib_stat.st_ino);
    }
    else
    {
        exit(112); // kitu atveju baigti programa ir isvesti 112 koda
    }

    printf("--3a--\n");
    // reikia patikrinti ar duotas pirmas argumentas yra biblioteka
    void *testlib = dlopen(argv[1], RTLD_LAZY | RTLD_LOCAL); // dlopen grazina pointeri jeigu duodas path yra biblioteka
    if (testlib == NULL)                                     // dlopen negrazino pointerio -> pirmas argumentas nera biblioteka
    {
        printf("%s nėra dinaminė biblioteka.\n", argv[1]);
        printf("--3c--\n");
        // jeigu pirmas argumentas nera dinamine biblioteka,
        // (zinodami is anksciau kad failas egzistuoja) reikia isvesti pirmus 5 baitus ir paskutinius 5 baitus
        int lib_deskr = open(argv[1], O_RDWR);
        if (lib_deskr == -1)
        {
            printf("%s neegzistuoja arba negalima jo atidaryti.\n", argv[1]);
            printf("Error %d: %s\n", errno, strerror(errno));
            exit(1);
        }
        // naudoju mmap
        void *file_mmap = mmap(NULL, lib_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, lib_deskr, 0);
        if (file_mmap == MAP_FAILED)
        {
            printf("Nepavyko prijungti pirmo failo atitinkancio kriterijus i procesu erdve.\n");
            printf("Error %d: %s\n", errno, strerror(errno));
            close(lib_deskr);
            exit(1);
        }
        // atspausdinti pirmus 5 baitus
        printf("Pirmi 5 baitai:");
        for (int i = 0; i < 5; i++)
        {
            // kiekviena baita konvertuoju i char masyva
            printf(" %d", ((unsigned char *)file_mmap)[i]);
        }
        // atspausdinti paskutinius 5 baitus
        int from = lib_stat.st_size - 6;
        int to = lib_stat.st_size - 1;
        printf(", paskutiniai 5 baitai:");
        for (int i = from; i < to; i++)
        {
            printf(" %d", ((unsigned char *)file_mmap)[i]);
        }
        printf("\n");
        // atjungti faila
        if (munmap(file_mmap, lib_stat.st_size) == -1)
        {
            printf("Nepavyko atjungti pirmo failo atitinkancio kriterijus i procesu erdve.\n");
            printf("Error %d: %s\n", errno, strerror(errno));
            close(lib_deskr);
            exit(1);
        }
        close(lib_deskr);
    }
    else
    {
        // dlopen grazino pointeri -> pirmas argumentas yra dinamine biblioteka
        printf("%s yra dinaminė biblioteka.\n", argv[1]);
        printf("--3b--\n");
        // bandom uzkrauti antram ir treciam argumentuose nurodytas funkcijas/kintamuosius
        // nezinojau kaip suzinoti ar argumente nurodytas dalykas
        // yra funkcija ar kintamasis dinaminei bibliotekoj, tai dariau asumpcija

        // jeigu bibliotekoj nera - tai reikia isvesti '<argumentas> bibliotekoje nera.'
        // jeigu bibliotekoj yra, tai reikia mums isvesti jo adresa procesu erdvej,
        // o ta galime pasidaryti konvertuodami pointerio adresa i longint
        *(void **)(&argv2_ptr) = dlsym(testlib, argv[2]);
        if (argv2_ptr == NULL)
        {

            printf("\"%s\" bibliotekoje nėra.\n", argv[2]);
        }
        else
        {
            printf("\"%s\" adresas: %ld \n", argv[2], (long int)argv2_ptr);
        }
        *(void **)(&argv3_ptr) = dlsym(testlib, argv[3]);
        if (argv3_ptr == NULL)
        {
            printf("\"%s\" bibliotekoje nėra.\n", argv[3]);
        }
        else
        {
            printf("\"%s\" adresas: %ld \n", argv[3], (long int)argv3_ptr);
        }

        // nepamirstam uzdaryti bibliotekos (pamirsau patikrinti ar tinkamai uzdare)
        dlclose(testlib);
    }

    printf("--3d--\n");
    // reikia vartotojo namu kataloge surasti failus, kurie yra susieti
    // kietuoju rysiu su bibliotekos failu (t.y. simbolinis failas su tuo paciu i-node berods)
    long maxPathLength = pathconf("/", _PC_PATH_MAX);

    if (maxPathLength == -1)
    {
        printf("Nepavyko nuskaityti maksimalaus kelio ilgio.\n");
        exit(1);
    }

    char *home_dir = getcwd(NULL, maxPathLength);
    if (home_dir == NULL)
    {
        printf("Nepavyko nuskaityti namų katalogo kelio.\n");
        exit(1);
    }

    printf("Einamasis katalogas: %s ", home_dir);

    // atidarom direktorija
    DIR *dir = opendir(home_dir);
    struct dirent *curr;
    struct stat curr_file_stat;

    char file_path[maxPathLength];
    int found = 0;

    // skaitom kiekviena faila direktorijoj
    while ((curr = readdir(dir)) != NULL)
    {
        //  isvalyti file_path char masyva, nes galimai ten jau yra kelias iki praeito failo
        // (as galeciau kurti char masyva kiekvienam cikle, ir tada nereiketu isvalyneti jo)
        memset(file_path, '\0', sizeof(file_path));

        // prideti katalogo kelia su failu
        strcat(file_path, home_dir);
        // cia sitas if'as is mano kd3 pavyzdines uzd. sprendimo,
        // nes ten galejo argumentuose arba nurodyti kelia su '/' gale arba ne,
        // bet kiek pamenu (rasau situos paaiskinimo komentarus kelios h po kolio) home_dir nepasibaigia su '/',
        // tai sita if galima nutrint ir palikti tiesiog strcat, kas prideda / prie char masyvo
        if (file_path[strlen(file_path) - 1] != '/')
            strcat(file_path, "/");
        // pridedame prie '<namu katalogas>/' failo varda
        strcat(file_path, curr->d_name);

        // irasom failo duomenis i struktura
        if (stat(file_path, &curr_file_stat) == -1)
        {
            printf("Klaida nuskaitant %s informacija\n", file_path);
            closedir(dir);
            exit(1);
        }

        // ispradziu patikrinam ar tai yra simbolinis failas ir tada ar i-node yra tas pats
        // (nesugalvojau tuo metu kaip kitaip tikrinti ar tai yra kieta nuoroda i biblioteka)
        if (S_ISLNK(curr_file_stat.st_mode) && curr_file_stat.st_ino == lib_stat.st_ino)
        {
            if (found == 0)
            {
                // mums reikia tik viena karta isspausinti sia eilute
                printf(". Šiame kataloge kietu ryšiu su %s susiję failai:", argv[1]);
            }
            found++;
            // o poto jau kiekvieno tokio failo pavadinima pridedam prie anos eilutes
            printf(" %s", curr->d_name);
        }
    }

    if (found == 0)
    {
        printf(". Šiame kataloge nėra kietu ryšiu su %s susijusių failų.\n", argv[1]);
    }
    closedir(dir);
    return 0;
}