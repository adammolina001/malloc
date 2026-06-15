#include <stddef.h> //Pour size_t
#include <unistd.h> //Pour sbrk
#include <stdio.h>  //Pour printf

typedef struct block_meta{
/*Structure block_meta qui indique : 
    - la taille du bloc 
    - si il est libre ou pas 
    - pointeur vers le suivant 
  En mémoire : [block_meta1][donnée de taille 1][block_meta2][donnée de taille 2]...*/
    
    size_t size;
    int isFree;
    struct block_meta* next_block;
}block_meta;

block_meta* global_base = NULL;  //== premier bloc, il n'y en a pas 

void my_memcpy(void* new_ptr, void* ptr, size_t block_size) {
/*Copie les donnée que contient un pointer vers un nouveau pointeur pour my_realloc
  On cast new_ptr et ptr en unsigned char pour les avoir en octets par octets*/
    unsigned char* ptrcpy = (unsigned char*) ptr;
    unsigned char* new_ptrcpy = (unsigned char*) new_ptr;
    for (size_t i = 0; i < block_size; i++) {
        new_ptrcpy[i] = ptrcpy[i];
    }
}

block_meta* find_free_block(size_t size_needed, block_meta* first_block, block_meta** last_block){
/*Regarde si un block de taille nécessaire est libre et le return si c'est le cas 
  sinon return NULL
  - last_block est un pointeur vers l'adresse d'un block et fait office de parameter output 
    donc il sera mis a jour dans le while et servira a avoir l'adresse du dernier block visité 
    si on ne trouve pas de block corespondant*/

    block_meta* current = first_block;
    while(current && (!current->isFree || (current->size < size_needed))) {
        *last_block = current;
        current = current->next_block;
    }
    return current;
}

block_meta* request_block(size_t size_needed, block_meta* last_block) {
/*Créer un nouveau block_meta et une zone de donnée de taille size_needed avec sbrk
  Ensuite met les bonnes valeurs a ce nouveau block_meta
  Enfin si c'est le premier block met a jour global_base
  - sbrk(t) demmande t mémoire a l'OS et renvoie un pointeur au début de type void* 
    donc on doit rajouter le cast (block_meta*) */

    block_meta* new_block = (block_meta*) sbrk(size_needed + sizeof(block_meta));
    if (new_block == (void*)-1) {
        fprintf(stderr, "Error: failed sbrk\n");    //Les messages d'erreur vont sur stderr
        return NULL;
    }

    new_block->size = size_needed;
    new_block->isFree = 0;
    new_block->next_block = NULL;

    if (last_block) {
        last_block->next_block = new_block;
    }

    if (!global_base) {
        global_base = new_block;
    }

    return new_block;
}

void* my_malloc(size_t size_needed) {
/*Regarde si il y a un bloque de taille sufisante avec find_free_block,
  le marque comme occupé et return l'adresse juste après le block_meta
  sinon fais une requete a l'OS avec request_block et return l'adresse après le block_meta
  si le request échoue on le return et malloc sera égal a NULL*/

    block_meta* last_block = NULL;
    block_meta* search = find_free_block(size_needed, global_base, &last_block);
    if (search) {
        search->isFree = 0;
        return (void*) (search + 1); //ici 1 = sizeof(block_meta)
    }
    
    block_meta* request = request_block(size_needed, last_block);
    if(request) {
        return (void*) (request + 1);
    }
    
    return request;
}

void my_free(void* ptr) {
/*ptr est un pointeur qui a été initialisé par malloc donc d'adresse block + 1
  my_free va revenir a l'adresse du block et le marquer comme libre isFree = 1*/
    block_meta* block_ptr = (block_meta*) ptr - 1;
    block_ptr->isFree = 1;

/*Deuxième partie le coalescing
  On va faire en sorte de combiner block_meta cote a cote si ils sont libres*/
    block_meta* next = block_ptr->next_block;
    while(next && (next->isFree == 1)) {
        block_ptr->size += next->size + sizeof(block_meta);
        block_ptr->next_block = next->next_block;
        next = block_ptr->next_block;
    }
}

void* my_realloc(void* ptr, size_t new_size) {
//Prend un poiteur qui a été initialisé avec malloc et lui alloue plus ou moins d'espace
    if (ptr == NULL) return my_malloc(new_size);
    if (new_size == 0) { my_free(ptr); return NULL;}

    block_meta* block_ptr = (block_meta*) ptr - 1; //Le block de ptr

//Si le block a une taille sufisante il n'y a rien a faire
//sinon on créer un nouveau poiteur dans une nouvelle zone, on y copie les donnée de ptr et on libere ptr
    if (block_ptr->size < new_size) {
        void* new_ptr = my_malloc(new_size);
        my_memcpy(new_ptr, ptr, block_ptr->size);
        my_free(ptr);

        return new_ptr;
    }
    return ptr;
}

int main () {
//Sert a tester nos fonctions
//On va essayer de stocké un int puis de l'écrire avec son adresse 
    int* test = (int*) my_malloc(sizeof(int));
    test[0] = 2;

    int* test2 = (int*) my_malloc(sizeof(int));
    test2[0] = 25;

    printf("\n");
    printf("int* test = (int*) my_malloc(sizeof(int));\ntest[0] = 2;\nint* test2 = (int*) malloc(sizeof(int));\ntest2[0] = 25;\n");
    printf("value: %d | Adress: %p\n", test[0], (void*) test);    //avec %p printf veut un void*
    printf("value: %d | Adress: %p\n", test2[0], (void*) test2);
    printf("\n");

    test = my_realloc(test, 3 * sizeof(int));

    printf("\n");
    printf("test = realloc(test, 3 * sizeof(int));\n");
    printf("value: %d | Adress: %p\n", test[0], (void*) test);    //avec %p printf veut un void*
    printf("value: %d | Adress: %p\n", test2[0], (void*) test2);
    printf("\n");

    test2 = my_realloc(test2, 2 * sizeof(int));
    
    printf("\n");
    printf("realloc(test2, 2 * sizeof(int));\n");
    printf("value: %d | Adress: %p\n", test[0], (void*) test);    //avec %p printf veut un void*
    printf("value: %d | Adress: %p\n", test2[0], (void*) test2);


    return 0;
}

