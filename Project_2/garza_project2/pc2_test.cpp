#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fstream>
#define NUM_THREADS 3       // number of player threads
#define NUM_PLAYERS 3       // number of players
#define NUM_ROUNDS 3        // total rounds
#define NUM_IN_SUIT 4       // 4 cards per suit

/**********************
    PTHREAD GLOBALS
***********************/
pthread_mutex_t mutex_deck_busy = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_dealer_done = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t win_cond = PTHREAD_COND_INITIALIZER;
pthread_t player_thread[NUM_THREADS];
pthread_t dealer_thread;
/**********************
    GLOBAL VARIABLES
***********************/

FILE * logfile;
int NUM_SUIT = 4;
int current_round = 1;
int test_value = 7;
int card_turn = 0;
//deck variables
const int NUM_CARDS = 52;
int deck[NUM_CARDS];
//points for top and bottom of deck
int *top_deck;
int *bottom_deck;
//
int seed = 0;
bool round_win = false;
//struct that creates virtual hand for each player
struct hand
    {
       int card_1, card_2;
    }; hand player_1, player_2, player_3;  // hands for the players
/************************
    FUNCTION PROTOTYPES
*************************/
void *dealer(void *arg);
void *player(void *player_ID);
void use_deck(long, hand);
void display_deck();

/************************
        MAIN
*************************/
int main(int argc, char *argv[])
{
    srand(time(NULL));
    // open the log file
    logfile = fopen("log.txt", "a");
    fprintf(logfile, "------------\nPairWar Game\n------------\n");
    seed = test_value;
    //seed = atoi(argv[1]);
    srand(seed);
    int cardVal = 0;
    int card = 0;
    while( (card < NUM_CARDS) && (cardVal < (NUM_CARDS/NUM_SUIT)) )
    {
      for( int i = 0; i < NUM_SUIT; i++ )
        {
            deck[card] = cardVal;
            card++;
        }
      cardVal++;
    }
    top_deck = deck;          // point to top of deck
    bottom_deck = deck + 51;  // point to bottom of deck
    //display deck
    display_deck();

    //launch the rounds
    while( current_round <= NUM_ROUNDS )
    {

        printf("ROUND #%d: \n", current_round);
        fprintf(logfile, "ROUND #%d: \n", current_round);
        // create dealer thread
        pthread_create(&dealer_thread, NULL, &dealer, NULL);
        // create player threads
        for( long i = 1; i <= NUM_THREADS; i++ )
        {
            pthread_create(&player_thread[i], NULL, &player, (void *)i);
        }
        // join threads so that function waits until all threads complete
        pthread_join(dealer_thread, NULL);
        for( int j = 0; j < 3; j++ )
        {
            pthread_join(player_thread[j], NULL);
        }
      current_round++;
      round_win = false;
    }
    // close the log file
    fclose(logfile);
    exit(EXIT_SUCCESS);
}

/************************
    Dealer Thread F
*************************/
void *dealer(void *arg)
{
    //0 means dealer id
    long player_id = 0;
    //0 means dealers turn
    card_turn = 0;
    //dealer needs null hand for parameter
    hand dealer_hand;
    //dealer uses deck
    use_deck(player_id, dealer_hand);

    // leave the dealer thread
    //lock exit
    pthread_mutex_lock(&mutex_dealer_done);
    while( !round_win )
    {
        pthread_cond_wait(&win_cond, &mutex_dealer_done);
    }
    //unlock exit
    pthread_mutex_unlock(&mutex_dealer_done);
    fprintf(logfile, "DEALER: Exits Round\n");
    pthread_exit(NULL);
}

/************************
    Player Thread F
*************************/
void *player(void *player_ID)
{
    //player id(1-3)
    long player_id = (long)player_ID;
    // assign hands to players based on which round is being played
    hand current_hand;
    if( current_round == 1 )
    {
      if( player_id == 1 )
        current_hand = player_1;
      else if( player_id == 2 )
        current_hand = player_2;
      else
        current_hand = player_3;
    }
    else if( current_round == 2 ){
      if( player_id == 2 )
        current_hand = player_1;
      else if( player_id == 3 )
        current_hand = player_2;
      else
        current_hand = player_3;
    }
    else if( current_round == 3 )
    {
      if( player_id == 3 )
        current_hand = player_1;
      else if( player_id == 1 )
        current_hand = player_2;
      else
        current_hand = player_3;
    }

    while( round_win == 0 )
    {
        //lock deck
        pthread_mutex_lock(&mutex_deck_busy);
        while( player_id != card_turn && round_win == 0 )
            {
                //call wait, to make players wait their turn
                pthread_cond_wait(&condition_var, &mutex_deck_busy);
            }
        if( round_win == 0 )
            {
                //players use deck
                use_deck(player_id, current_hand);
            }
        // unlock the deck
        pthread_mutex_unlock(&mutex_deck_busy);
    }
    // leave the player thread
    fprintf(logfile, "PLAYER #%ld: Exits Round\n", player_id);
    pthread_exit(NULL);
}

/************************
    Dealer Use's Deck
*************************/
void use_deck(long player_id, hand current_hand)
{
    if( player_id == 0 )
    {
        //dealer uses deck
        //shuffles
        fprintf(logfile, "DEALER: Shuffle\n");
        for( int i = 0; i < (NUM_CARDS - 1); i++ )
        {
          int random = i + (rand() % (NUM_CARDS - i));
          int temp = deck[i];
          deck[i] = deck[random];
          deck[random] = temp;
        }
        //deal first card from top of deck, increment top of deck
        fprintf(logfile, "DEALER: Deal\n");
        player_1.card_1 = *top_deck; top_deck = top_deck + 1;
        player_2.card_1 = *top_deck; top_deck = top_deck + 1;
        player_3.card_1 = *top_deck; top_deck = top_deck + 1;
    }
    //player uses deck


    else
    {
        // show hand
        fprintf(logfile, "PLAYER #%ld: Hand %d \n", player_id, current_hand.card_1);

        // draw a card
        current_hand.card_2 = *top_deck,
        top_deck = top_deck + 1;
        fprintf(logfile, "PLAYER #%ld: Draws %d \n", player_id,current_hand.card_2);

        // show hand
        printf("HAND %d %d\n", current_hand.card_1, current_hand.card_2);
        fprintf(logfile, "PLAYER #%ld: Hand %d %d\n", player_id, current_hand.card_1, current_hand.card_2);

        // compare cards in hand
        if( current_hand.card_1 == current_hand.card_2 )
        {
            // if match, winner
            printf("WIN yes\n");
            fprintf(logfile, "PLAYER #%ld: Wins\n", player_id);
            round_win = true;
            // signal dealer to exit
            pthread_cond_signal(&win_cond);
        }
        else
        {
            // if the cards don't match, then discard a card
            printf("WIN no\n");
            // shift cards in deck to make room for discard
            top_deck = top_deck - 1;
            int *ptr = top_deck;
            while( ptr != bottom_deck )
            {
                *ptr = *(ptr + 1);
                ptr = ptr + 1;
            }
            // randomly select discard and place in the bottom of the deck
            int discard = rand() % 2;
            if( discard == 0 )
            {
                fprintf(logfile, "PLAYER #%ld: Discards %d \n", player_id, current_hand.card_1);
                *bottom_deck = current_hand.card_1;
                //move card 2 into card 1
                current_hand.card_1 = current_hand.card_2;
            }
            else
            {
                fprintf(logfile, "PLAYER #%ld: Discards %d \n", player_id, current_hand.card_2);
                *bottom_deck = current_hand.card_2;
            }
            // print the contents of the deck
            display_deck();
        }
    }
    //next players turn
    card_turn ++;
    //if card_turn is > 3, all players went, start at player 1 again
    if( card_turn > NUM_THREADS )
        card_turn = 1;
    //broadcast that deck is available
    pthread_cond_broadcast(&condition_var);

}

/************************
        Display Deck
*************************/
void display_deck()
{
    //pointer traverses deck array, prints data
    printf("DECK: ");
    fprintf(logfile, "DECK: ");
    int *ptr = top_deck;

    while( ptr != bottom_deck )
    {
        printf("%d ", *ptr);
        fprintf(logfile, "%d ", *ptr);
        ptr++;
        if( ptr == bottom_deck )
        {
            printf("%d \n", *ptr);
            fprintf(logfile, "%d \n", *ptr);
        }
    }
}

