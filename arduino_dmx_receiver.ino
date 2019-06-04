#include <Conceptinetics.h>


#define NUM_CHANNELS 2
#define POWER_DELAY 300

#define POWER_PIN 2
#define UP_PIN 3
#define DOWN_PIN 4 
#define DMX_TIMEOUT 150000 //in ms



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

/** Turn up relais on/off */
void setUp(bool on);

/* Turn down relais on/off */
void setDown(bool on);

/** Error handler. Turn relais of and freeze mcu */
void error();

/**callback to signal dmx frame received */
void onFrameReceiveComplete();


DMX_Slave dmxSlave(NUM_CHANNELS);
State state = STATE_OFF;
bool frameReceived = false;
unsigned long iterationsSinceLastPacket = 0;

void setup() {
  frameReceived = false;
  dmxSlave.enable();
  dmxSlave.setStartAddress(10);
  dmxSlave.onReceiveComplete(onFrameReceiveComplete);
  pinMode(LED_BUILTIN, OUTPUT);

  //this block has to happen before pinmodes are set, otherwise pins will be on for a short time!
  setPower(false);
  delay(POWER_DELAY);
  setUp(false);
  setDown(false);
  state = STATE_OFF;
  
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);

}


// the loop function runs over and over again forever
void loop() 
{ 
  //do nothing until we receive at least one good dmx frame
  if(!frameReceived)     
    return;

  //FIXME !!!!
  //this is a  HACK to count cycles since last dmx packet received. Cannot use millis() because it breaks the dmx library... too lazy to use another timer just count iterations for now
  ++iterationsSinceLastPacket;
  
  const uint8_t channelOn = dmxSlave.getChannelValue(1);
  const uint8_t channelDir = dmxSlave.getChannelValue(2);
  Command cmd = getCommand(channelOn, channelDir);
 
  switch(state)
  {
    case STATE_OFF:
      off(cmd);
      break;
    case STATE_UP:
      up(cmd);
      break;
    case STATE_DOWN:
      down(cmd);
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
      setUp(true);
      setDown(false);
      delay(POWER_DELAY);
      setPower(true);
      delay(POWER_DELAY);
      state = STATE_UP;
      return;
    case CMD_DOWN:
      setDown(true);
      setUp(false);
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
  //switch of power first, wait, then switch off direction relais
  setPower(false);
  delay(POWER_DELAY);
  setUp(false);
  setDown(false);
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
  //turn motor off if dmx is disconnected
  if(iterationsSinceLastPacket > DMX_TIMEOUT)
  {
    return CMD_OFF;
  }
  
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

void setUp(bool on)
{
  if(!on)
  {
      digitalWrite(UP_PIN, HIGH);
  }
  else
  {
      digitalWrite(UP_PIN, LOW);
  }  
}

void setDown(bool on)
{
  if(!on)
  {
      digitalWrite(DOWN_PIN, HIGH);
  }
  else
  {
      digitalWrite(DOWN_PIN, LOW);
  }  
}

void onFrameReceiveComplete(unsigned short channelsReceived)
{
  frameReceived = true;
  iterationsSinceLastPacket = 0;
  //lastFrameReceivedTime = millis();
}
