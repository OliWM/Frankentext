#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_WORD_COUNT 15'000
#define MAX_SUCCESSOR_COUNT MAX_WORD_COUNT / 2 // there's no way we would encounter a text in which 
                     // a single word had 50% of all words a successors so this is                                       \
                     // a safe limit and saves some memory

char book[] = {
#embed "pg84.txt" /// Stores the content of the file as an array of chars.
    , '\0'};      /// Makes `book` a string.

/// Array of tokens registered so far.
/// No duplicates are allowed.
char *tokens[MAX_WORD_COUNT];
// betyder det er en array af pointers. Så hvert
// element i arrayen er en pointer til et ord
/// `tokens`'s current size
size_t tokens_size = 0;

/// Array of successor tokens
/// One token can have many successor tokens. `succs[x]` corresponds to
/// `token[x]`'s successors.
/// We store directly tokens instead of token_ids, because we will directly
/// print them. If we wanted to delete the book, then it would make more sense
/// to store `token_id`s
char *succs[MAX_WORD_COUNT][MAX_SUCCESSOR_COUNT]; // 2D array af alle ord og alle deres
                                  // prede-/successors på den anden led
/// `succs`'s current size
size_t succs_sizes[MAX_WORD_COUNT]; // array of ints telling the size of each
                                    // token in succs (how many successors each
                                    // token succs[i] has)

/// Overwrites non-printable characters in `book` with a space.
/// Non-printable characters may lead to duplicates like
/// `"\xefthe" and "the"` even both print `the`.
void replace_non_printable_chars_with_space() {
  for (size_t i = 0; i < sizeof(book); i++) {
    if (!isprint(book[i])) { // isprint returns true if the char is printable.
                             // Iterates through all chars in the text.
      book[i] = ' ';
    };
  }
}

/// Returns the id (index) of the token, creating it if necessary.
///
/// Returns token id if token exists in \c tokens, otherwise creates a new entry
/// in \c tokens and returns its token id.
///
/// \param token token to look up (or insert)
/// \return Index of `token` in \c tokens array
size_t token_id(char *token) { // Iterer over hvert eneste token og ser om den token vi
                        // har fat i allerede har været der.. nok ikk verdens
                        // mest effektive måde at gøre det på - evt. et hashmap?
                        // Nok gjort sådan her af pædagogiske årsager
  size_t id;
  for (id = 0; id < tokens_size; ++id) {
    if (strcmp(tokens[id], token) == 0) {
      return id;
    }
  }
  tokens[id] = token;
  ++tokens_size;
  return id;
}

/// Appends the token \c succ to the successors list of \c token.
void append_to_succs(char *token, char *succ) {
  // tager den token[i] vi arbejder med nu + det successor ord
  // vi har fundet. Er succs bare token[i+1] og token[i-1]?
  auto next_empty_index_ptr = &succs_sizes[token_id(token)];
  // Pointer til hvor mange succs et bestemt token har. Auto er Lokal variabel, der forsvinder når funktionen slutter. AFAIK
  // Unødvendigt i moderne c fordi alle variabler i funktioner automatisk er auto. Medmindre gekko har en compiler der tager
  // det efterfølgenes type ved auto

  if (*next_empty_index_ptr >= MAX_SUCCESSOR_COUNT) {
    // tjekker om vi har fyldt en tokens succs liste
    // (om vi skriver ud over arrayens grænse)
    printf("Successor array full.");
    exit(EXIT_FAILURE);
  }

  succs[token_id(token)][(*next_empty_index_ptr)++] = succ;
  // finder det sted i vores 2d array succs[token][næste ledige plads
  // i token's succs liste] og sætter = succ (ordet)
}

/// Creates tokens on \c book and fills \c tokens and \c succs using
/// the functions \c token_id and \c append_to_succs.
// noget med at finde mellemrum, bruge strlen
void tokenize_and_fill_succs(char *delimiters, char *str) {
  char *token = strtok(str, delimiters); // laver det første ord til en token
  char *prev = NULL; // så den ikk prøver at appende til første ord -1 (ville nok give et eller andet memory shit)
  while (token != NULL && tokens_size < MAX_WORD_COUNT) {  //efter sidste token er NULL
    token_id(token); //tager den token - hvis den er ny registrer den den i tokens
    if (prev != NULL) {
      append_to_succs(prev, token); //tager den nuværende token og appender til den foregående (fordi nuværende er foregåenes successor)
    }
    prev = token; //Gemmer nuværende som previous, så vi er klar til næste run
    token = strtok(NULL, delimiters); // strtok husker hvor den sidst var. Så vi
                                      // behøber ikke sige den skal starte i
                                      // book igen det ved den godt. Så nu
                                      // starter den bare ved sidste NULL, som
                                      // er det den selv har tilføjet
                                      // så den gør næste ord til token.
  }
}

  /// Returns last character of a string
  char last_char(char *str) {
    int len = strlen(str);
    if (len == 0) {return '\0';} //betyder at det er en empty string
    return str[len-1];
  }

/// Returns whether the token ends with `!`, `?` or `.`.
bool token_ends_a_sentence(char *token) {
  char to_be_checked = last_char(token);
  if (to_be_checked == '.' || to_be_checked == '!' ||
      to_be_checked ==
          '?') { // kunne nok skrives mere clean da return to_be_checked == '.' fx jo ville returnere true eller false allerede.
    return true;
  } else {
    return false;
  }
}

/// Returns a random `token_id` that corresponds to a `token` that starts with a
/// capital letter.
/// Uses \c tokens and \c tokens_size.
size_t random_token_id_that_starts_a_sentence() {
//no srand as we assume we do it when running the programme from main
  while(true){ // would get stuck if text had no tokens with a capital letter.
    size_t random_token_id = rand() % tokens_size ;
    int first_letter = *tokens[random_token_id];
      if (first_letter >= 'A' && first_letter <= 'Z') {
        return random_token_id;
      }
  }
}

/// Generates a random sentence using \c tokens, \c succs, and \c succs_sizes.
/// The sentence array will be filled up to \c sentence_size-1 characters using
/// random tokens until:
/// - a token is found where \c token_ends_a_sentence
/// - or more tokens cannot be concatenated to the \c sentence anymore.
/// Returns the filled sentence array.
///
/// @param sentence array what will be used for the sentence.
//
//                  Will be overwritten. Does not have to be initialized.
/// @param sentence_size
/// @return input sentence pointer
char *generate_sentence(char *sentence, size_t sentence_size) {
  // size_t = størrelsen på objektet, kan være max memory
  // størrelse (derfor -1 så der er plads til '\0'), kan
  // kun være positive værdier (kun brugt til memory)
  size_t current_token_id = random_token_id_that_starts_a_sentence();
  // Starter med at være den her -
  // derfra bliver den successors
  auto token = tokens[current_token_id];

  sentence[0] = '\0'; // afslutningen som strcat flytter til sidst

  strcat(sentence, token);
  // merger det nuværende token ind i sætningen (sætter på
  // slutningen, flytter \0 til efter), tror ikk den tilføjer space
  if (token_ends_a_sentence(token))
    return sentence;

  // Concatenates random successors to the sentence as long as
  // `sentence` can hold them.
  while (true) {
    // hent antal successors for det nuværende token
    size_t succ_count = succs_sizes[current_token_id];
    if (succ_count == 0)
      break; // der er ingen successors, vi stopper

    // vælg en random successor
    char *successor_to_add = succs[current_token_id][rand() % succ_count];

    // beregn om der er plads i sentence array
    size_t len_needed = strlen(successor_to_add) + 1; // +1 for space
    if (strlen(sentence) + len_needed >= sentence_size - 1)
      break; // for langt, vi stopper

    strcat(sentence, " ");              // tilføj space
    strcat(sentence, successor_to_add); // tilføj successor til sentence

    // opdater current token id til successorens id
    current_token_id = token_id(successor_to_add);
    token = successor_to_add;

    // stop hvis successor ender en sætning
    if (token_ends_a_sentence(token))
      break;
  }

  return sentence;
}

int main() {
  replace_non_printable_chars_with_space();

  char *delimiters = " \n\r";
  tokenize_and_fill_succs(delimiters, book);

  char sentence[1000];
  srand(time(nullptr)); // Be random each time we run the program

  // Generate sentences until we find a question sentence.
  do {
    generate_sentence(sentence, sizeof sentence);
  } while (last_char(sentence) != '?');
  puts(sentence);
  puts("");

  // Initialize `sentence` and then generate sentences until we find a sentence
  // ending with an exclamation mark.
  do {
    generate_sentence(sentence, sizeof sentence);
  } while (last_char(sentence) != '!');
  puts(sentence);
}