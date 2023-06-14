# RUN THE MAKE COMMMAND TO RUN THE GAME

# need all executables to run game
run: game barbarian wizard rogue 
	./game

# Compile and link the game and dungeon process
game: game.c dungeon.o barbarian wizard rogue
	gcc game.c dungeon.o -o game -lrt -pthread

# Compile and link barbarian process
barbarian: barbarian.c 
	gcc barbarian.c -o barbarian -lrt -pthread

# Compile and link wizard process
wizard: wizard.c
	gcc wizard.c -o wizard -lrt -pthread

# Compile and link Rogue process
rogue: rogue.c	
	gcc rogue.c -o rogue -lrt -pthread








