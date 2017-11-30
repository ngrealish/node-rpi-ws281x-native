#include <nan.h>

#include <v8.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <algorithm>

extern "C" {
#include "rpi_ws281x/ws2811.h"
}

using namespace v8;

#define DEFAULT_TARGET_FREQ 800000
#define DEFAULT_GPIO_PIN 18
#define DEFAULT_DMANUM 5

#define PARAM_FREQ 1
#define PARAM_DMANUM 2
#define PARAM_GPIONUM 3
#define PARAM_COUNT 4
#define PARAM_INVERT 5
#define PARAM_BRIGHTNESS 6
#define PARAM_STRIP_TYPE 7

ws2811_t ws281x =
    {
        .freq = DEFAULT_TARGET_FREQ,
        .dmanum = DEFAULT_DMANUM,
        .channel =
            {
                [0] =
                    {
                        .gpionum = DEFAULT_GPIO_PIN,
                        .count = 0,
                        .invert = 0,
                        .brightness = 255,
                        .strip_type = 0,
                    },
                [1] =
                    {
                        .gpionum = 0,
                        .count = 0,
                        .invert = 0,
                        .brightness = 0,
                    },
            },
};

/**
 * ws281x.setParam(param:Number, value:Number)
 * wrap setting global params in ws2811_t
 */
void setParam(const Nan::FunctionCallbackInfo<v8::Value> &info)
{
  if (info.Length() != 2)
  {
    Nan::ThrowTypeError("setParam(): expected two params");
    return;
  }

  if (!info[0]->IsNumber())
  {
    Nan::ThrowTypeError("setParam(): expected argument 1 to be the parameter-id");
    return;
  }

  if (!info[1]->IsNumber())
  {
    Nan::ThrowTypeError("setParam(): expected argument 2 to be the value");
    return;
  }

  int param = info[0]->Int32Value();
  int value = info[1]->Int32Value();

  switch (param)
  {
  case PARAM_FREQ:
    ws281x.freq = value;
    break;
  case PARAM_DMANUM:
    ws281x.dmanum = value;
    break;

  default:
    Nan::ThrowTypeError("setParam(): invalid parameter-id");
    return;
  }

  info.GetReturnValue.SetUndefined();
}
/**
 * ws281x.setChannelParam(channel:Number, param:Number, value:Number)
 *
 * wrap setting params in ws2811_channel_t
 */
void setChannelParam(const Nan::FunctionCallbackInfo<v8::Value> &info)
{
  if (info.Length() != 3)
  {
    Nan::ThrowTypeError("setChannelParam(): missing argument");
    return;
  }

  // retrieve channelNumber from argument 1
  if (!info[0]->IsNumber())
  {
    Nan::ThrowTypeError("setChannelParam(): expected argument 1 to be the channel-number");
    return;
  }

  int channelNumber = info[0]->Int32Value();
  if (channelNumber > 1 || channelNumber < 0)
  {
    Nan::ThrowError("setChannelParam(): invalid chanel-number");
    return;
  }

  if (!info[1]->IsNumber())
  {
    Nan::ThrowTypeError("setChannelParam(): expected argument 2 to be the parameter-id");
    return;
  }

  if (!info[2]->IsNumber())
  {
    Nan::ThrowTypeError("setChannelParam(): expected argument 3 to be the value");
    return;
  }

  ws2811_channel_t channel = ws281x.channel[channelNumber];
  int param = info[1]->Int32Value();
  int value = info[2]->Int32Value();

  switch (param)
  {
  case PARAM_GPIONUM:
    channel.gpionum = value;
    break;
  case PARAM_COUNT:
    channel.count = value;
    break;
  case PARAM_INVERT:
    channel.invert = value;
    break;
  case PARAM_BRIGHTNESS:
    channel.brightness = (uint8_t)value;
    break;
  case PARAM_STRIP_TYPE:
    channel.strip_type = value;
    break;

  default:
    Nan::ThrowTypeError("setChannelParam(): invalid parameter-id");
    return;
  }

  info.GetReturnValue().SetUndefined();
}

/**
 * ws281x.setChannelData(channel:Number, buffer:Buffer)
 *
 * wrap copying data to ws2811_channel_t.leds
 */
void setChannelData(const Nan::FunctionCallbackInfo<v8::Value> &info)
{
  if (info.Length() != 2)
  {
    Nan::ThrowTypeError("setChannelData(): missing argument.");
    return;
  }

  // retrieve channelNumber from argument 1
  if (!info[0]->IsNumber())
  {
    Nan::ThrowTypeError("setChannelData(): expected argument 1 to be the channel-number.");
    return;
  }

  int channelNumber = info[0]->Int32Value();
  if (channelNumber > 1 || channelNumber < 0)
  {
    Nan::ThrowError("setChannelData(): invalid chanel-number");
    return;
  }
  ws2811_channel_t channel = ws281x.channel[channelNumber];

  // retrieve buffer from argument 2
  if (!node::Buffer::HasInstance(info[1]))
  {
    Nan::ThrowTypeError("setChannelData(): expected argument 2 to be a Buffer");
    return;
  }
  Local<Object> buffer = info[0]->ToObject();
  uint32_t *data = (uint32_t *)node::Buffer::Data(buffer);

  if (channel.count == 0 || channel.leds == NULL)
  {
    Nan::ThrowError("setChannelData(): channel not ready");
    return;
  }

  int numBytes = std::min(
      node::Buffer::Length(buffer),
      sizeof(ws2811_led_t) * ws281x.channel[0].count);

  memcpy(channel.leds, data, numBytes);

  info.GetReturnValue.SetUndefined();
}

/**
 * ws281x.init()
 *
 * wrap ws2811_init()
 */
void init(const Nan::FunctionCallbackInfo<v8::Value> &info)
{
  ws2811_return_t ret;

  ret = ws2811_init(&ws281x);
  if (ret != WS2811_SUCCESS)
  {
    Nan::ThrowError(ws2811_get_return_t_str(ret));
    return;
  }

  info.GetReturnValue.SetUndefined();
}

/**
 * ws281x.render()
 *
 * wrap ws2811_wait() and ws2811_render()
 */
void render(const Nan::FunctionCallbackInfo<v8::Value> &info)
{
  ws2811_return_t ret;

  ret = ws2811_wait(&ws281x);
  if (ret != WS2811_SUCCESS)
  {
    Nan::ThrowError(ws2811_get_return_t_str(ret));
    return;
  }

  ret = ws2811_render(&ws281x);
  if (ret != WS2811_SUCCESS)
  {
    Nan::ThrowError(ws2811_get_return_t_str(ret));
    return;
  }

  info.GetReturnValue.SetUndefined();
}

/**
 * ws281x.finalize()
 *
 * wrap ws2811_wait() and ws2811_fini()
 */
void finalize(const Nan::FunctionCallbackInfo<v8::Value> &info)
{
  ws2811_return_t ret;

  ret = ws2811_wait(&ws281x);
  if (ret != WS2811_SUCCESS)
  {
    Nan::ThrowError(ws2811_get_return_t_str(ret));
    return;
  }

  ws2811_fini(&ws281x);
  info.GetReturnValue.SetUndefined();
}

/**
 * initializes the module.
 */
void initialize(Local<Object> exports)
{
  NAN_EXPORT(exports, setParam);
  NAN_EXPORT(exports, setChannelParam);
  NAN_EXPORT(exports, setChannelData);
  NAN_EXPORT(exports, init);
  NAN_EXPORT(exports, render);
  NAN_EXPORT(exports, finalize);
}

NODE_MODULE(rpi_ws281x, initialize)

// vi: ts=2 sw=2 expandtab
