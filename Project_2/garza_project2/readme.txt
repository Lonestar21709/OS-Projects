The 5 runs were done with seeds from (1-5).

I have two versions of my program.

Testing version has a default seed of 6, and can be run from the ide. 
While the pc2_garza.cpp runs from the Linux Command line, with the following steps.

1) on command line type: gcc pr2_garza.cpp -lpthread
2) type: ./a.out (seed value)

Step 1 will create the a.out file. 
Then step 2 runs the program witht the seed value, and creates a log.txt file in the directory.

So there are a total of 4 threads, 1 dealer and 3 player threads.  My deck is an array of 52 numbers that fill up from (0-12)X4.  Each player and the dealer has a struct that creates a virtual hand that can hold 2 cards.  My main file receives the seed, generates the deck, points to top and bottom of the deck array.  Then I create the threads and join them.  The dealer uses the deck and it checks who is using it.  If it is the dealers turn then he must shuffle and deal the cards, else its a players turn and this is where they draw, discard and possibly win the game. Dealer locks, then waits for players to play the game and win condition to occur to unlock, and exit the round. While the dealer is waiting, he signals the players to use the deck until one of them wins the game.  THe game continues for 3 rounds, and then the program exits. 

So basically there is always someone using the deck, while everyone else waits for their turn or conditions to happen.  I use mutex's to lock and unlock player and dealer threads into their sequences.  The players turn with the deck is determined by a variable, and the player_id determines between dealer and player. Also when the player discards a card, it goes into the bottom of the deck and the rest of the deck shifts by 1.  