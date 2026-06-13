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

block_meta* global_base = NULL;  //= premier bloc, il n'y en a pas 

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

    block_meta* new = (block_meta*) sbrk(size_needed + sizeof(block_meta));
    if (new == (void*)-1) {
        printf("Error: failed sbrk");
        return NULL;
    }

    new->size = size_needed;
    new->isFree = 0;
    new->next_block = NULL;

    if (last_block) {
        last_block->next_block = new;
    }

    if (!global_base) {
        global_base = new;
    }

    return new;
}

void* malloc(size_t size_needed) {
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

void free(void* ptr) {
/*ptr est un pointeur qui a été initialisé par malloc donc d'adresse block + 1
  free va revenir a l'adresse du block et le marquer comme libre isFree = 1*/
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

int main () {
//Sert a tester nos fonctions
//On va essayer de stocké un int puis de l'écrire avec son adresse 
    int* test = (int*) malloc(sizeof(int));
    test[0] = 2;

    int* test2 = (int*) malloc(sizeof(int));
    test2[0] = 25;

    printf("value: %d | Adress: %p\n", test[0], (void*) test);    //avec %p printf veut un void*
    printf("value: %d | Adress: %p\n", test2[0], (void*) test2);

    free(test2);

    int* test3 = (int*) malloc(sizeof(int));
    test3[0] = 43;
    
    printf("value: %d | Adress: %p\n", test3[0], (void*) test3);

    free(test3);
    free(test); //Il y a le coalexcing

    block_meta* block_test = (block_meta*) test - 1;
    int size = block_test->size;
    printf("size: %d | adress: %p\n", size, (void*)block_test);

    return 0;
}

