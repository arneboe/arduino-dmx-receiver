#include <Conceptinetics.h>


#define NUM_CHANNELS 2
#define POWER_DELAY 300 //time to wait until for relais to stabilize (debounce)

#define POWER_PIN 2
#define DIR_PIN_1 3
#define DIR_PIN_2 4 

#define DMX_TIMEOUT 150000 //in iterations



enum State
{
  STATE_OFF,
  STATE_UP,
  STATE_DOWN
};

enum Command
{
  CMD_OFF,
  CMD_UP,
  CMD_DOWN 
};

enum Direction
{
  DIR_UP,
  DIR_DOWN
};



/** off() is called while the state machine is in state off */
void off(Command cmd);
/**up() is called while the state machine is in state up */
void up(Command cmd);
/** down() is called while the state machine is in state down */
void down(Command cmd);

/** switches the state machine into state off */
void goToOff();

/** Convert dmx channel values into command */
Command getCommand(uint8_t channelOn, uint8_t channelDir);

/** Turn power relai on/off */
void setPower(bool on);

/** set direction relais */
void setDirection(Direction dir);


/** Error handler. Turn relais of and freeze mcu */
void error();

/**callback to signal dmx frame received */
void onFrameReceiveComplete();


DMX_Slave dmxSlave(NUM_CHANNELS);

State state = STATE_OFF; //internal state of the state machine

bool frameReceived = false; //false until we receive at least one frame
unsigned long iterationsSinceLastPacket = 0; //ghetto timer indicating iterations since last dmx packet received

Command currentCommand; //command currently executed by the sate machine

void setup() {
  frameReceived = false;
  dmxSlave.enable();
  dmxSlave.setStartAddress(10);
  dmxSlave.onReceiveComplete(onFrameReceiveComplete);
  pinMode(LED_BUILTIN, OUTPUT);

  //this block has to happen before pinmodes are set, otherwise pins will be on for a short time!
  setPower(false);
  delay(POWER_DELAY);
  setDirection(DIR_UP);
  state = STATE_OFF;
  currentCommand = CMD_OFF;
  
  pinMode(POWER_PIN, OUTPUT);
  pinMode(DIR_PIN_1, OUTPUT);
  pinMode(DIR_PIN_2, OUTPUT);

}

// the loop function runs over and over again forever
void loop() 
{ 
  //only update cmd if a complete frame has been received
  if(frameReceived)
  {
    frameReceived = false;
    iterationsSinceLastPacket = 0;
    const uint8_t channelDir = dmxSlave.getChannelValue(2);
    const uint8_t channelOn = dmxSlave.getChannelValue(1);
    currentCommand = getCommand(channelOn, channelDir);  
  }

  //this is a  HACK to count cycles since last dmx packet received. Cannot use millis() because it breaks the dmx library... too lazy to use another timer just count iterations for now
  ++iterationsSinceLastPacket;
  
  //turn motor off if dmx is disconnected
  if(iterationsSinceLastPacket > DMX_TIMEOUT)
  {
    return currentCommand = CMD_OFF;
  }
  
  switch(state)
  {
    case STATE_OFF:
      off(currentCommand);
      break;
    case STATE_UP:
      up(currentCommand);
      break;
    case STATE_DOWN:
      down(currentCommand);
      break;
    default:
      error();
  }
}

void off(Command cmd)
{ 
  switch(cmd)
  {
    case CMD_OFF:
      //nothing to do, are already in off
      return;
      
    case CMD_UP:
      setDirection(DIR_UP);
      delay(POWER_DELAY);
      setPower(true);
      delay(POWER_DELAY);
      state = STATE_UP;
      return;
      
    case CMD_DOWN:
      setDirection(DIR_DOWN);
      delay(POWER_DELAY);
      setPower(true);
      delay(POWER_DELAY);
      state = STATE_DOWN;
      return;

    default:
      error();  
  }
}

void up(Command cmd)
{
  switch(cmd)
  {
    //both off/and down turn the motor off! in case of down it will turn up on next iteration
    case CMD_OFF:
    case CMD_DOWN:
      goToOff();
      return;
    case CMD_UP:
      return;//already in up
    default:
      error();
  }
}

void down(Command cmd)
{
  switch(cmd)
  {
    case CMD_OFF:
    case CMD_UP:
      goToOff();
      return;
    case CMD_DOWN:
      return;
    default:
      error();
  }
}

void goToOff()
{
  //just switch off power, direction relais stay on
  setPower(false);
  delay(POWER_DELAY);
  state = STATE_OFF;  
}

void error()
{
  //error case, turn of motor and hang!
  while(true)
  {
    setPower(false);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);                    
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);    
  }
}


Command getCommand(uint8_t channelOn, uint8_t channelDir)
{ 
  if(channelOn > 128)
  {
    if(channelDir < 50)
    {
      return CMD_DOWN;
    }
    else if(channelDir >=50 && channelDir < 150)
    {
      return CMD_OFF;
    }
    else if(channelDir >= 150)
    {
      return CMD_UP;
    }
    else
    {
      error();
    }
  }
  else
  {
    return CMD_OFF;
  } 
  
  error();
}


void setPower(bool on)
{
  if(!on)
  {
      digitalWrite(POWER_PIN, HIGH);
  }
  else
  {
      digitalWrite(POWER_PIN, LOW);
  }
}

void setDirection(Direction dir)
{
  switch(dir)
  {
    case DIR_UP:
      digitalWrite(DIR_PIN_1, HIGH);
      digitalWrite(DIR_PIN_2, HIGH);
      return;
    case DIR_DOWN:
      digitalWrite(DIR_PIN_1, LOW);
      digitalWrite(DIR_PIN_2, LOW);
      return;
    default:
      error();
  } 
}


void onFrameReceiveComplete(unsigned short channelsReceived)
{
  frameReceived = true;
}
