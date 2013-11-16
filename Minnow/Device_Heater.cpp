/*
 Minnow Pacemaker client firmware.
    
 Copyright (C) 2013 Robert Fairlie-Cuninghame

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include "Device_Heater.h"
#include "Device_TemperatureSensor.h"
#include "temperature_ISR.h"
#include "response.h"
#include "initial_pin_state.h"

extern uint8_t checkAnalogOrDigitalPin(uint8_t pin);

uint8_t Device_Heater::num_heaters = 0;

Device_Heater::HeaterInfo *Device_Heater::heater_info_array;

uint8_t Device_Heater::soft_pwm_device_bitmask;
SoftPwmState *Device_Heater::soft_pwm_state;

float Device_Heater::PID_dT = 0.0; 

//
// Methods
//

uint8_t Device_Heater::Init(uint8_t num_devices)
{
  uint8_t i;
  
  if (num_devices == num_heaters)
    return APP_ERROR_TYPE_SUCCESS;
  if (num_heaters != 0)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_ALREADY_INITIALIZED));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }

  uint8_t *memory = (uint8_t*)malloc(num_devices * sizeof(HeaterInfo));
  if (memory == 0)
  {
    generate_response_msg_addPGM(PMSG(MSG_ERR_INSUFFICIENT_MEMORY));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }

  heater_info_array = (HeaterInfo *) memory;
     
  for (i=0; i<num_devices; i++)
  {
    heater_info_array[i].device_number = i;
    heater_info_array[i].heater_pin = 0xFF;
    heater_info_array[i].temp_sensor = 0xFF;
    heater_info_array[i].control_mode = HEATER_CONTROL_MODE_INVALID;
    heater_info_array[i].max_temp = SENSOR_TEMPERATURE_INVALID;
    heater_info_array[i].target_temp = SENSOR_TEMPERATURE_INVALID;
    heater_info_array[i].power_on_level = DEFAULT_HEATER_POWER_ON_LEVEL;
  }
  
  soft_pwm_state = 0;
  soft_pwm_device_bitmask = 0;
 
  num_heaters = num_devices;
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_Heater::SetHeaterPin(uint8_t device_number, uint8_t heater_pin)
{
  if (device_number >= num_heaters)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  
  uint8_t retval = checkAnalogOrDigitalPin(heater_pin);
  if (retval != APP_ERROR_TYPE_SUCCESS)
    return retval;
  
  heater_info_array[device_number].heater_pin = heater_pin;
  
  set_initial_pin_state(heater_pin, INITIAL_PIN_STATE_LOW);
  
  if (DEFAULT_HEATER_USE_SOFT_PWM)
  {
    uint8_t retval = EnableSoftPwm(device_number, true);
    if (retval != APP_ERROR_TYPE_SUCCESS)
      return retval;
  }
  
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_Heater::SetTempSensor(uint8_t heater_device_number, uint8_t sensor_device_number)
{
  if (heater_device_number >= num_heaters)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

  if (sensor_device_number >= Device_TemperatureSensor::GetNumDevices())
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;

  if (PID_dT == 0.0)
  {
    // calculate sampling period of the temperature sensor routine
    // (if there are more than 8 temperature sensors, it takes
    // more than 8ms to read them all)
    PID_dT = (float)(OVERSAMPLENR * min(8, Device_TemperatureSensor::GetNumDevices())) / (F_CPU / 64.0 / 256.0);
  }    
  
  heater_info_array[heater_device_number].temp_sensor = sensor_device_number;
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_Heater::SetControlMode(uint8_t device_number, uint8_t mode)
{
  if (device_number >= num_heaters)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

  if (mode != HEATER_CONTROL_MODE_PID && mode != HEATER_CONTROL_MODE_BANG_BANG)
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;

  if (heater_info_array[device_number].control_mode != HEATER_CONTROL_MODE_INVALID)
  {
    if (heater_info_array[device_number].control_mode != mode)
    {
      generate_response_msg_addPGM(PMSG(MSG_ERR_ALREADY_INITIALIZED));
      return PARAM_APP_ERROR_TYPE_FAILED;
    }
    return APP_ERROR_TYPE_SUCCESS;
  }
    
  if (mode == HEATER_CONTROL_MODE_BANG_BANG)
  {
    // set defaults
    heater_info_array[device_number].control_info.bangbang.hysteresis = DEFAULT_BANG_BANG_HYSTERESIS;
  }
  else if (mode == HEATER_CONTROL_MODE_PID)
  {
    if (heater_info_array[device_number].control_mode != HEATER_CONTROL_MODE_PID)
    {
      PidInfo *pid_info = (PidInfo *)malloc(sizeof(PidInfo));
      if (pid_info == 0)
      {
        generate_response_msg_addPGM(PMSG(MSG_ERR_INSUFFICIENT_MEMORY));
        return PARAM_APP_ERROR_TYPE_FAILED;
      }
      pid_info->functional_range = DEFAULT_PID_FUNCTIONAL_RANGE;
      pid_info->advanced_K1 = DEFAULT_PID_K1;
      pid_info->advanced_max_pid_power_level = DEFAULT_MAX_PID_POWER_LEVEL;
      pid_info->advanced_integral_drive_max = DEFAULT_INTEGRAL_DRIVE_MAX;
      pid_info->Kp = 0.0;
      pid_info->Ki = 0.0;
      pid_info->Kd = 0.0;
      heater_info_array[device_number].control_info.pid = pid_info;
    }
  }
  else
  {
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  }
    
  heater_info_array[device_number].control_mode = mode;
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_Heater::SetPowerOnLevel(uint8_t device_number, uint8_t level)
{
  if (device_number >= num_heaters)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

  heater_info_array[device_number].power_on_level = level;
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_Heater::EnableSoftPwm(uint8_t device_number, bool enable)
{
  if (device_number >= num_heaters || device_number >= sizeof(soft_pwm_device_bitmask)*8)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

  if (heater_info_array[device_number].heater_pin == 0xFF)
  {
    generate_response_msg_addPGM(PMSG(ERR_MSG_INVALID_PIN_NUMBER));
    return PARAM_APP_ERROR_TYPE_FAILED;
  }

  if (soft_pwm_state == 0 && enable)
  {
    soft_pwm_state = (SoftPwmState *)malloc(sizeof(SoftPwmState));
    if (soft_pwm_state == 0 
        || !soft_pwm_state->Init(min(num_heaters,sizeof(soft_pwm_device_bitmask)*8)))
    {
      generate_response_msg_addPGM(PMSG(MSG_ERR_INSUFFICIENT_MEMORY));
      return PARAM_APP_ERROR_TYPE_FAILED;
    }
  }
  
  if (soft_pwm_state != 0)
  {
    if (!soft_pwm_state->EnableSoftPwm(device_number, heater_info_array[device_number].heater_pin, enable))
    {
      generate_response_msg_addPGM(PMSG(ERR_MSG_INVALID_PIN_NUMBER));
      return PARAM_APP_ERROR_TYPE_FAILED;
    }
  }
    
  if (enable)
    soft_pwm_device_bitmask |= (1<<device_number);
  else
    soft_pwm_device_bitmask &= ~(1<<device_number);
    
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_Heater::SetMaxTemperature(uint8_t device_number, float temp)
{
  if (device_number >= num_heaters)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;
  
  if (temp <= 0)
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;
  
  heater_info_array[device_number].max_temp = temp;
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_Heater::SetBangBangHysteresis(uint8_t device_number, uint8_t temp_range)
{
  if (device_number >= num_heaters)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

  HeaterInfo *heater_info = &heater_info_array[device_number];
  if (heater_info->control_mode != HEATER_CONTROL_MODE_BANG_BANG)
    return PARAM_APP_ERROR_TYPE_INCORRECT_MODE;
    
  heater_info->control_info.bangbang.hysteresis = temp_range;
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_Heater::SetPidFunctionalRange(uint8_t device_number, uint8_t value)
{
  if (device_number >= num_heaters)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

  HeaterInfo *heater_info = &heater_info_array[device_number];
  if (heater_info->control_mode != HEATER_CONTROL_MODE_PID)
    return PARAM_APP_ERROR_TYPE_INCORRECT_MODE;
    
  heater_info->control_info.pid->functional_range = value;
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_Heater::SetPidDefaultKp(uint8_t device_number, float value)
{
  if (device_number >= num_heaters)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

  HeaterInfo *heater_info = &heater_info_array[device_number];
  if (heater_info->control_mode != HEATER_CONTROL_MODE_PID)
    return PARAM_APP_ERROR_TYPE_INCORRECT_MODE;
    
  heater_info->control_info.pid->Kp = value;
  UpdatePidDerivedConfig(heater_info);
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_Heater::SetPidDefaultKi(uint8_t device_number, float value)
{
  if (device_number >= num_heaters)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

  HeaterInfo *heater_info = &heater_info_array[device_number];
  if (heater_info->control_mode != HEATER_CONTROL_MODE_PID)
    return PARAM_APP_ERROR_TYPE_INCORRECT_MODE;
    
  heater_info->control_info.pid->Ki = value * PID_dT;
  UpdatePidDerivedConfig(heater_info);
  return APP_ERROR_TYPE_SUCCESS;
}

uint8_t Device_Heater::SetPidDefaultKd(uint8_t device_number, float value)
{
  if (device_number >= num_heaters)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

  HeaterInfo *heater_info = &heater_info_array[device_number];
  if (heater_info->control_mode != HEATER_CONTROL_MODE_PID)
    return PARAM_APP_ERROR_TYPE_INCORRECT_MODE;
    
  heater_info->control_info.pid->Kd = value / PID_dT;
  UpdatePidDerivedConfig(heater_info);
  return APP_ERROR_TYPE_SUCCESS;
}


uint8_t Device_Heater::ValidateTargetTemperature(uint8_t device_number, float temp)
{
  if (device_number >= num_heaters
      || heater_info_array[device_number].heater_pin == 0xFF)
    return PARAM_APP_ERROR_TYPE_INVALID_DEVICE_NUMBER;

  if (!Device_TemperatureSensor::IsInUse(heater_info_array[device_number].temp_sensor)
      || heater_info_array[device_number].max_temp == SENSOR_TEMPERATURE_INVALID
      || (heater_info_array[device_number].control_mode != HEATER_CONTROL_MODE_PID
            && heater_info_array[device_number].control_mode != HEATER_CONTROL_MODE_BANG_BANG))
    return PARAM_APP_ERROR_TYPE_FAILED;
    
  if (heater_info_array[device_number].control_mode == HEATER_CONTROL_MODE_PID)
  {
    PidInfo *pid_info = heater_info_array[device_number].control_info.pid;
    if (pid_info->Kp == 0.0 || pid_info->Ki == 0.0 || pid_info->Kd == 0.0)
      return PARAM_APP_ERROR_TYPE_FAILED;
  }
  
  if (temp > heater_info_array[device_number].max_temp || temp < 0)
    return PARAM_APP_ERROR_TYPE_BAD_PARAMETER_VALUE;

  return APP_ERROR_TYPE_SUCCESS;
}

void Device_Heater::UpdateHeaters()
{
  HeaterInfo *heater_info = heater_info_array;
  for (uint8_t i=0; i<num_heaters; i++)
  {
    float target_temp = heater_info->target_temp;
    if (target_temp != SENSOR_TEMPERATURE_INVALID)
    {
      // Check if temperature is within the correct range
      float current_temp = Device_TemperatureSensor::ReadCurrentTemperature(heater_info->temp_sensor);
      if (current_temp > heater_info->max_temp || current_temp == SENSOR_TEMPERATURE_INVALID)
      {
        // TODO handle heater error.
        if (current_temp == SENSOR_TEMPERATURE_INVALID)
          ERROR("Heater Read Error: ");
        else
          ERROR("Heater overtemp Error: ");
        ERRORLN((int)i);
        heater_info->target_temp = SENSOR_TEMPERATURE_INVALID;
        SetHeaterPower(heater_info, 0);
      }
      else if (heater_info->control_mode == HEATER_CONTROL_MODE_BANG_BANG)
      {
        // Bang Bang Mode
        if (heater_info->is_heating)
        {
          if (current_temp > target_temp + heater_info->control_info.bangbang.hysteresis)
          {
            SetHeaterPower(heater_info, 0);
          }
        }
        else 
        {
          if (current_temp < target_temp - heater_info->control_info.bangbang.hysteresis)
          {
            SetHeaterPower(heater_info, heater_info->power_on_level);
          }
        }
      }
      else if (heater_info->control_mode == HEATER_CONTROL_MODE_PID)
      {
        // PID mode
        PidInfo *pid_info = heater_info->control_info.pid;
        uint8_t pid_power;
        
        float pid_error = target_temp - current_temp;
        if (pid_error > pid_info->functional_range)
        {
          pid_power = heater_info->power_on_level;
          pid_info->state_iState = 0.0; // reset PID state
        }
        else if(pid_error < -(pid_info->functional_range)) 
        {
          pid_power = 0;
          pid_info->state_iState = 0.0; // reset PID state
        }
        else 
        {
          // update and constrain iState
          pid_info->state_iState += pid_error;
          if (pid_info->state_iState > pid_info->iState_max)
            pid_info->state_iState = pid_info->iState_max;
          else if (pid_info->state_iState < 0.0)
            pid_info->state_iState = 0.0;
            
          pid_info->state_dTerm = ((current_temp - pid_info->last_temp) * pid_info->KdK2) 
                                    + (pid_info->advanced_K1 * pid_info->state_dTerm);

          float pTerm = pid_info->Kp * pid_error;
          float iTerm = pid_info->Ki * pid_info->state_iState;
          float pid_output = pTerm + iTerm - pid_info->state_dTerm;
          
          if (pid_output >= (float)pid_info->advanced_max_pid_power_level)
            pid_power = pid_info->advanced_max_pid_power_level;
          else if (pid_output <= 0.0)
            pid_power = 0;
          else 
            pid_power = (uint8_t)pid_output;

          #ifdef PID_DEBUG
          DEBUG(" PIDDEBUG ");
          DEBUG(e);
          DEBUG(": Input ");
          DEBUG(current_temp);
          DEBUG(" Output ");
          DEBUG(pid_output);
          DEBUG(" pTerm ");
          DEBUG(pTerm);
          DEBUG(" iTerm ");
          DEBUG(iTerm);
          DEBUG(" dTerm ");
          DEBUG(pid_info->state_dTerm);  
          #endif //PID_DEBUG
        }
        pid_info->last_temp = current_temp; 
        SetHeaterPower(heater_info, pid_power);
      }
    }
    heater_info += 1;
  }
}

void Device_Heater::UpdatePidDerivedConfig(HeaterInfo *heater_info)
{
  if (heater_info->control_mode != HEATER_CONTROL_MODE_PID)
    return;
  PidInfo *pid_info = heater_info->control_info.pid;
  
  pid_info->iState_max = pid_info->advanced_max_pid_power_level / pid_info->Ki;  
  pid_info->KdK2 = pid_info->Kd * (1.0 - pid_info->advanced_K1);  
  
  InitializePidState(heater_info);
}

//
// Copied largely unchanged from Marlin
//
void Device_Heater::DoPidAutotune(uint8_t device_number, float temp, int ncycles)
{
  float input = 0.0;
  int cycles = 0;
  bool heating = true;

  unsigned long temp_millis = millis();
  unsigned long t1=temp_millis;
  unsigned long t2=temp_millis;
  long t_high = 0;
  long t_low = 0;

  long bias, d;
  float Ku, Tu;
  float Kp, Ki, Kd;
  float max = 0;
  float min = 10000;
  uint8_t p;
  uint8_t i;

  extern volatile bool temp_meas_ready;
  
  DEBUGLNPGM("PID Autotune start");
  
  // switch off all heaters during auto-tune
  for (i=0; i<Device_Heater::GetNumDevices(); i++)
  {
    if (Device_Heater::IsInUse(i))
      Device_Heater::SetHeaterPower(&heater_info_array[device_number], 0);
  }

  HeaterInfo *heater_info = &heater_info_array[device_number];
  
  bias = d = p = heater_info->power_on_level;
  SetHeaterPower(heater_info, p);

  for(;;) 
  {
    if(temp_meas_ready == true) 
    { // temp sample ready
      Device_TemperatureSensor::UpdateTemperatureSensors();

      input = Device_Heater::ReadCurrentTemperature(device_number);

      max=max(max,input);
      min=min(min,input);
      if(heating == true && input > temp) 
      {
        if(millis() - t2 > 5000) 
        { 
          heating=false;
          p = bias - d;
          SetHeaterPower(heater_info, p);
          t1=millis();
          t_high=t1 - t2;
          max=temp;
        }
      }
      if(heating == false && input < temp) 
      {
        if(millis() - t1 > 5000) {
          heating=true;
          t2=millis();
          t_low=t2 - t1;
          if(cycles > 0) {
            bias += (d*(t_high - t_low))/(t_low + t_high);
            bias = constrain(bias, 20 ,heater_info->power_on_level-20);
            if (bias > heater_info->power_on_level/2) 
              d = heater_info->power_on_level - 1 - bias;
            else 
              d = bias;

            DEBUGPGM(" bias: "); DEBUG_F(bias, DEC);
            DEBUGPGM(" d: "); DEBUG_F(d, DEC);
            DEBUGPGM(" min: "); DEBUG(min);
            DEBUGPGM(" max: "); DEBUGLN(max);
            
            if(cycles > 2) 
            {
              Ku = (4.0*d)/(3.14159*(max-min)/2.0);
              Tu = ((float)(t_low + t_high)/1000.0);
              DEBUGPGM(" Ku: "); DEBUG(Ku);
              DEBUGPGM(" Tu: "); DEBUGLN(Tu);
              Kp = 0.6*Ku;
              Ki = 2*Kp/Tu;
              Kd = Kp*Tu/8;
              DEBUGLNPGM(" Clasic PID ");
              DEBUGPGM(" Kp: "); DEBUGLN(Kp);
              DEBUGPGM(" Ki: "); DEBUGLN(Ki);
              DEBUGPGM(" Kd: "); DEBUGLN(Kd);
              /*
              Kp = 0.33*Ku;
              Ki = Kp/Tu;
              Kd = Kp*Tu/3;
              DEBUGLNPGM(" Some overshoot ")
              DEBUGPGM(" Kp: "); DEBUGLN(Kp);
              DEBUGPGM(" Ki: "); DEBUGLN(Ki);
              DEBUGPGM(" Kd: "); DEBUGLN(Kd);
              Kp = 0.2*Ku;
              Ki = 2*Kp/Tu;
              Kd = Kp*Tu/3;
              DEBUGLNPGM(" No overshoot ")
              DEBUGPGM(" Kp: "); DEBUGLN(Kp);
              DEBUGPGM(" Ki: "); DEBUGLN(Ki);
              DEBUGPGM(" Kd: "); DEBUGLN(Kd);
              */
            }
          }
          p = bias + d;
          SetHeaterPower(heater_info, p);
          cycles++;
          min=temp;
        }
      } 
    }
    if(input > (temp + 20)) 
    {
      ERRORLNPGM("PID Autotune failed! Temperature too high");
      return;
    }
    if(millis() - temp_millis > 2000) 
    {
      DEBUGLNPGM("ok T:");
      DEBUG(input);   
      DEBUGPGM(" @:");
      DEBUGLN(p);       
      temp_millis = millis();
    }
    if(((millis() - t1) + (millis() - t2)) > (10L*60L*1000L*2L)) 
    {
      ERRORLNPGM("PID Autotune failed! timeout");
      return;
    }
    if(cycles > ncycles) 
    {
      DEBUGLNPGM("PID Autotune finished! Record the last Kp, Ki and Kd constants");
      return;
    }
  }
}
