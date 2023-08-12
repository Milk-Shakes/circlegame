
  byte Enemies[][20]={

    {200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200}, //Level1
    
    {10, 13, 15, 200, 25, 28, 200, 200, 51, 54, 200, 200, 200, 70, 73, 200, 200, 200, 200, 200}, //Level2
    
    {0, 1, 2, 3, 15, 16, 17, 30, 31, 32, 33, 34, 35, 41, 42, 43, 200, 200, 200, 200} //Level3
  
  };

  byte MovingEnemies[][16]={
    
    {/*clockwise*/200, 200, 200, 200, 200, 200, 200, 200 /*Counterclockwise*/, 200, 200, 200, 200, 200, 200, 200, 200}, //Level1
    
    {/*clockwise*/200, 200, 200, 200, 200, 200, 200, 200 /*Counterclockwise*/, 200, 200, 200, 200, 200, 200, 200, 200}, //Level2
    
    {/*clockwise*/200, 200, 200, 200, 200, 200, 200, 50 /*Counterclockwise*/, 51, 200, 200, 200, 200, 200, 200, 200}, //Level3
  
  };

  byte Finish_Position[]={
    59, 42, 75
  };

int EnemyLevelSize(byte LevelNumber){
  
  int EnemyArraySize = 0;
  EnemyArraySize = (sizeof(Enemies[LevelNumber])/sizeof(Enemies[LevelNumber][0]));
  return EnemyArraySize;
  
}

int MovingEnemyLevelSize(byte LevelNumber){
  
  int MovingEnemyArraySize = 0;
  MovingEnemyArraySize = (sizeof(MovingEnemies[LevelNumber])/sizeof(MovingEnemies[LevelNumber][0]));
  return MovingEnemyArraySize;
  
}

byte LevelRequest(byte Levelnumber, byte arrayrequest, byte EntityType) {
 
 byte entityposition;
 if (EntityType == 1) {
  entityposition = Enemies[Levelnumber][arrayrequest];
 } else if ( EntityType == 2) {
  entityposition = MovingEnemies[Levelnumber][arrayrequest];
 } else if ( EntityType == 3) {
  entityposition = Finish_Position[Levelnumber];
 }
 return entityposition;
  
}
