/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */  
#include <msp430.h>
#include <stdlib.h>
#include <stdio.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "buzzer.h"

#define GREEN_LED BIT6


AbRect paddle = {abRectGetBounds, abRectCheck, {20,3}};
AbRect ball = {abRectGetBounds, abRectCheck, {2,2}};
AbRArrow rightArrow = {abRArrowGetBounds, abRArrowCheck, 30};

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 1, screenHeight/2 - 5}
};

Layer layer4 = {
  (AbShape *)&rightArrow,
  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_WHITE,
  0
};
  

Layer layer3 = {		/**< Layer with an orange circle */
  (AbShape *)&ball,
  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_WHITE,
  0,
};


Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, (screenHeight/2)+4},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GREEN,
  &layer3
};

Layer p1 = {		/**< Layer with a red square */
  (AbShape *)&paddle,
  {screenWidth/2, screenHeight-10}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_VIOLET,
  &fieldLayer,
};

Layer p0 = {		/**< Layer with an orange circle */
  (AbShape *)&paddle,
  {(screenWidth/2), 20}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_ORANGE,
  &p1,
};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
MovLayer ml4 = { &layer4, {1,0}, 0}; //arrow
MovLayer ml3 = { &layer3, {3,10}, 0 };//ball is life
MovLayer ml1 = { &p1, {0,0}, &ml3 }; //player paddle
MovLayer ml0 = { &p0, {2,0}, &ml1 };//cpu paddle 

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */

  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  



//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */





unsigned char noise = 0, sDir = 0;
char P1S[6] = {'P','1',':','0','0','\0'};
char CPU[7] = {'C','P','U',':','0','0','\0'};
void mlAdvance(MovLayer *real, Region *fence)
{
  Vec2 newPos;

  Vec2 newPadPos;
  Vec2 newPad2Pos;
  Vec2 newBallPos;
  Region shapePadBoundary;
  Region shapePad2Boundary;
  Region shapeBallBoundary;
  
  u_char axis;
  Region shapeBoundary;
  MovLayer *ml;
  MovLayer *ball = real->next->next;

  int obj = 1;
  Region ballBoundary;
  //Vec2 newBallPos;
    
  for (ml = real; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);

  
    vec2Add(&newPadPos, &ml1.layer->posNext, &ml1.velocity);
    vec2Add(&newPad2Pos, &ml0.layer->posNext, &ml0.velocity);
    vec2Add(&newBallPos, &ml3.layer->posNext, &ml3.velocity);
    abShapeGetBounds(ml1.layer->abShape, &newPadPos, &shapePadBoundary);
    abShapeGetBounds(ml0.layer->abShape, &newPad2Pos, &shapePad2Boundary);
    abShapeGetBounds(ml3.layer->abShape, &newBallPos, &shapeBallBoundary);
    for (axis = 0; axis < 2; axis ++) {
      if((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis])){
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
	if(axis == 1){
	  noise = 2;
	  
	  if((P1S[4]) == '9'){
	    P1S[3] = (char)((int)P1S[3]+1);
	    P1S[4] = '0';
	  }else{
	    P1S[4] = (char)((int)P1S[4]+1);
	  }
	}
      }
      else if ((shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis])) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
	if(axis == 1){
	  noise = 2;

	  if((CPU[5]) == '9'){
	    CPU[4] = (char)((int)CPU[4]+1);
	   CPU[5] = '0';
	  }else{
	    CPU[5] = (char)((int)CPU[5]+1);
	  }
	}
      }

      if((shapeBallBoundary.topLeft.axes[0] > shapePadBoundary.topLeft.axes[0]) && (shapeBallBoundary.botRight.axes[0] < shapePadBoundary.botRight.axes[0]) && (shapeBallBoundary.topLeft.axes[1] == shapePadBoundary.botRight.axes[1]) ){
	int velocity = ml3.velocity.axes[1] = -10;
	newBallPos.axes[1] += (2*velocity);
	noise = 1;
      }
      if((shapeBallBoundary.topLeft.axes[0] > shapePad2Boundary.topLeft.axes[0]) && (shapeBallBoundary.botRight.axes[0] < shapePad2Boundary.botRight.axes[0]) && (shapeBallBoundary.botRight.axes[1] == shapePad2Boundary.topLeft.axes[1]) ){
	int velocity = ml3.velocity.axes[1] = 10;
	newBallPos.axes[1] += (2*velocity);
	noise = 1;
      }
      
      /**< if outside of fence */
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}











u_int bgColor = COLOR_BLACK;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */


/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);
  buzzer_init();
  shapeInit();

  layerInit(&p0);
  layerDraw(&p0);


  layerGetBounds(&fieldLayer, &fieldFence);
  

  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */
  
   
  for(;;) { 
    drawString5x7(0,0, P1S, COLOR_PURPLE, COLOR_BLACK);
    drawString5x7(85,0, CPU, COLOR_ORANGE, COLOR_BLACK);
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    movLayerDraw(&ml0, &p0);
  }
}
unsigned char paused = 0;
/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  int yBall = ml3.layer->pos.axes[0];
  int yPaddle = ml0.layer->pos.axes[0]; 
  if(yBall < yPaddle){
    ml0.velocity.axes[0] = -1;
  }
  else{
    ml0.velocity.axes[0] = 1;
  }
  unsigned int temp = p2sw_read();
  ml1.velocity.axes[0] = 0;
  
  if(temp == 5){//sw2 and 4
    P1S[4] = '0';
    P1S[3] = '0';
    CPU[4] = '0';
    CPU[5] = '0';
  }if(temp == 7){//sw4
    ml1.velocity.axes[0] = 2;
  }if(temp == 11){//sw3
    paused = 1;
  }if(temp == 13){//sw2
    paused = 0;
  }if(temp == 14){//sw1
    ml1.velocity.axes[0] = -2;
  }
  
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  if(!paused){
    count++;
    if ((count == 15)) {
      mlAdvance(&ml0, &fieldFence);
      if(noise == 1){
	buzzer_set_period(4000);
	noise = 0;
      }else if(noise == 2){
	buzzer_set_period(10000);
	noise = 0;
      }else{
	buzzer_set_period(0);
      }
      redrawScreen = 1;
      count = 0;
    }
  }
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
  
}
