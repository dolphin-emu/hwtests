#include <gctypes.h>
#include <wiiuse/wpad.h>
#include "common/hwtests.h"

static void lwzTest()
{
  START_TEST();

  u32 values[] = {
      0xFFFFFFFF, 0x00000000, 0x41414141, 0x11111111,
  };

  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
  {
    u32 result = 0;
    u32 address = (u32)&values[i];
    asm("lwz %0, 0(%1)" : "=r"(result) : "r"(address));
    DO_TEST(result == values[i], "lwz({}):\n"
                                 "\tgot {}\n"
                                 "\texpected {}",
            i, result, values[i]);
  }
  END_TEST();
}

static void lwzuTest()
{
  START_TEST();

  u32 values[] = {
      0xFFFFFFFF, 0x00000000, 0x41414141, 0x11111111,
  };

#define CONST_LWZU(offset, index)                                                                  \
  do                                                                                               \
  {                                                                                                \
    u32 result = 0;                                                                                \
    u32 address = (u32)&values[0];                                                                 \
    asm("lwzu %0, " #offset "(%1)" : "=r"(result), "+r"(address));                                 \
    DO_TEST(result == values[index], "lwzu({}):\n"                                                 \
                                     "\tgot {}\n"                                                  \
                                     "\texpected {}",                                              \
            index, result, values[index]);                                                         \
    DO_TEST(address == ((u32)&values[0] + offset), "lwzu({}):\n"                                   \
                                                   "\tgot 0x{:08x}\n"                              \
                                                   "\texpected 0x{:08x}",                          \
            index, address, ((u32)&values[0] + offset));                                           \
  } while (0)

  CONST_LWZU(0, 0);
  CONST_LWZU(4, 1);
  CONST_LWZU(8, 2);
  CONST_LWZU(12, 3);
  END_TEST();
}

static void lwzxTest()
{
  START_TEST();

  u32 values[] = {
      0xFFFFFFFF, 0x00000000, 0x41414141, 0x11111111,
  };

  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
  {
    u32 result = 0;
    u32 address = (u32)&values[0];
    u32 offset = (u32)&values[i] - address;
    asm("lwzx %0, %1, %2" : "=r"(result), "+r"(offset) : "r"(address));
    DO_TEST(result == values[i], "lwzx({}):\n"
                                 "\tgot {}\n"
                                 "\texpected {}",
            i, result, values[i]);
  }
  END_TEST();
}

static void lwzuxTest()
{
  START_TEST();

  u32 values[] = {
      0xFFFFFFFF, 0x00000000, 0x41414141, 0x11111111,
  };

  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
  {
    u32 result = 0;
    u32 address = (u32)&values[0];
    u32 offset = (u32)&values[i] - address;
    asm("lwzux %0, %1, %2" : "=r"(result), "+r"(offset) : "r"(address));
    DO_TEST(result == values[i], "lwzux({}):\n"
                                 "\tgot {}\n"
                                 "\texpected {}",
            i, result, values[i]);
    DO_TEST(offset == (u32)&values[i], "lwzux({}):\n"
                                       "\tgot 0x{:08x}\n"
                                       "\texpected 0x{:08x}",
            i, offset, (u32)&values[i]);
  }
  END_TEST();
}

static void lhzTest()
{
  START_TEST();

  u16 values[] = {
      0xFFFF, 0xF000, 0x0000, 0x0F0F, 0xF0F0, 0x8888,
  };

  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
  {
    u16 result = 0;
    u32 address = (u32)&values[i];
    asm("lhz %0, 0(%1)" : "=r"(result) : "r"(address));
    DO_TEST(result == values[i], "lhz({}):\n"
                                 "\tgot {}\n"
                                 "\texpected {}",
            i, result, values[i]);
  }
  END_TEST();
}

static void lhzxTest()
{
  START_TEST();

  u16 values[] = {
      0xFFFF, 0xF000, 0x0000, 0x0F0F, 0xF0F0, 0x8888,
  };

  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
  {
    u16 result = 0;
    u32 address = (u32)&values[0];
    u32 offset = (u32)&values[i] - address;
    asm("lhzx %0, %1, %2" : "=r"(result) : "r"(address), "r"(offset));
    DO_TEST(result == values[i], "lhzx({}):\n"
                                 "\tgot {}\n"
                                 "\texpected {}",
            i, result, values[i]);
  }
  END_TEST();
}

static void lhzuTest()
{
  START_TEST();

  u16 values[] = {
      0xFFFF, 0xF000, 0x0000, 0x0F0F, 0xF0F0, 0x8888,
  };

#define CONST_LHZU(offset, index)                                                                  \
  do                                                                                               \
  {                                                                                                \
    u16 result = 0;                                                                                \
    u32 address = (u32)&values[0];                                                                 \
    asm("lhzu %0, " #offset "(%1)" : "=r"(result), "+r"(address));                                 \
    DO_TEST(result == values[index], "lhzu({}):\n"                                                 \
                                     "\tgot {}\n"                                                  \
                                     "\texpected {}",                                              \
            index, result, values[index]);                                                         \
    DO_TEST(address == ((u32)&values[0] + offset), "lhzu({}):\n"                                   \
                                                   "\tgot 0x{:08x}\n"                              \
                                                   "\texpected 0x{:08x}",                          \
            index, address, ((u32)&values[0] + offset));                                           \
  } while (0)

  CONST_LHZU(0, 0);
  CONST_LHZU(2, 1);
  CONST_LHZU(4, 2);
  CONST_LHZU(6, 3);
  CONST_LHZU(8, 4);
  CONST_LHZU(10, 5);
  END_TEST();
}

static void lhzuxTest()
{
  START_TEST();

  u16 values[] = {
      0xFFFF, 0xF000, 0x0000, 0x0F0F, 0xF0F0, 0x8888,
  };
  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
  {
    u16 result = 0;
    u32 address = (u32)&values[0];
    u32 offset = (u32)&values[i] - address;
    asm("lhzux %0, %1, %2" : "=r"(result), "+r"(offset) : "r"(address));
    DO_TEST(result == values[i], "lhzux({}):\n"
                                 "\tgot {}\n"
                                 "\texpected {}",
            i, result, values[i]);
    DO_TEST(offset == (u32)&values[i], "lhzux({}):\n"
                                       "\tgot 0x{:08x}\n"
                                       "\texpected 0x{:08x}",
            i, offset, (u32)&values[i]);
  }
  END_TEST();
}

static void lhaTest()
{
  START_TEST();

  u16 values[] = {
      0xFFFF, 0xF000, 0x0000, 0x0F0F, 0xF0F0, 0x8888,
  };

#define CONST_LHA(offset, index)                                                                   \
  do                                                                                               \
  {                                                                                                \
    u32 result = 0;                                                                                \
    u32 address = (u32)&values[0];                                                                 \
    u32 expected = (u32)(s32)(s16)values[index];                                                   \
    asm("lha %0, " #offset "(%1)" : "=r"(result), "+r"(address));                                  \
    DO_TEST(result == expected, "lha({}):\n"                                                       \
                                "\tgot {}\n"                                                       \
                                "\texpected {}",                                                   \
            index, result, expected);                                                              \
  } while (0)

  CONST_LHA(0, 0);
  CONST_LHA(2, 1);
  CONST_LHA(4, 2);
  CONST_LHA(6, 3);
  CONST_LHA(8, 4);
  CONST_LHA(10, 5);
  END_TEST();
}

static void lhaxTest()
{
  START_TEST();

  u16 values[] = {
      0xFFFF, 0xF000, 0x0000, 0x0F0F, 0xF0F0, 0x8888,
  };

  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
  {
    u32 result = 0;
    u32 address = (u32)&values[0];
    u32 offset = (u32)&values[i] - address;
    u32 expected = (u32)(s32)(s16)values[i];
    asm("lhax %0, %1, %2" : "=r"(result) : "r"(address), "r"(offset));
    DO_TEST(result == expected, "lhax({}):\n"
                                "\tgot {}\n"
                                "\texpected {}",
            i, result, expected);
  }
  END_TEST();
}

static void lhauTest()
{
  START_TEST();

  u16 values[] = {
      0xFFFF, 0xF000, 0x0000, 0x0F0F, 0xF0F0, 0x8888,
  };

#define CONST_LHAU(offset, index)                                                                  \
  do                                                                                               \
  {                                                                                                \
    u32 result = 0;                                                                                \
    u32 address = (u32)&values[0];                                                                 \
    u32 expected = (u32)(s32)(s16)values[index];                                                   \
    asm("lhau %0, " #offset "(%1)" : "=r"(result), "+r"(address));                                 \
    DO_TEST(result == expected, "lhau({}):\n"                                                      \
                                "\tgot {}\n"                                                       \
                                "\texpected {}",                                                   \
            index, result, expected);                                                              \
    DO_TEST(address == ((u32)&values[0] + offset), "lhau({}):\n"                                   \
                                                   "\tgot 0x{:08x}\n"                              \
                                                   "\texpected 0x{:08x}",                          \
            index, address, ((u32)&values[0] + offset));                                           \
  } while (0)

  CONST_LHAU(0, 0);
  CONST_LHAU(2, 1);
  CONST_LHAU(4, 2);
  CONST_LHAU(6, 3);
  CONST_LHAU(8, 4);
  CONST_LHAU(10, 5);
  END_TEST();
}

static void lhauxTest()
{
  START_TEST();

  u16 values[] = {
      0xFFFF, 0xF000, 0x0000, 0x0F0F, 0xF0F0, 0x8888,
  };

  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
  {
    u32 result = 0;
    u32 address = (u32)&values[0];
    u32 offset = (u32)&values[i] - address;
    u32 expected = (u32)(s32)(s16)values[i];
    asm("lhaux %0, %1, %2" : "=r"(result), "+r"(address) : "r"(offset));
    DO_TEST(result == expected, "lhaux({}):\n"
                                "\tgot {}\n"
                                "\texpected {}",
            i, result, expected);
    DO_TEST(address == ((u32)&values[0] + offset), "lhaux({}):\n"
                                                   "\tgot 0x{:08x}\n"
                                                   "\texpected 0x{:08x}",
            i, offset, ((u32)&values[0] + offset));
  }
  END_TEST();
}

static void lbzTest()
{
  START_TEST();

  u8 values[256];
  for (int i = 0; i < 256; ++i)
    values[i] = i;

  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
  {
    u8 result = 0;
    u32 address = (u32)&values[i];
    asm("lbz %0, 0(%1)" : "=r"(result) : "r"(address));
    DO_TEST(result == values[i], "lbz({}):\n"
                                 "\tgot {}\n"
                                 "\texpected {}",
            i, result, values[i]);
  }
  END_TEST();
}

static void lbzxTest()
{
  START_TEST();

  u8 values[256];
  for (int i = 0; i < 256; ++i)
    values[i] = i;

  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
  {
    u8 result = 0;
    u32 address = (u32)&values[0];
    u32 offset = (u32)&values[i] - address;
    asm("lbzx %0, %1, %2" : "=r"(result) : "r"(address), "r"(offset));
    DO_TEST(result == values[i], "lbzx({}):\n"
                                 "\tgot {}\n"
                                 "\texpected {}",
            i, result, values[i]);
  }
  END_TEST();
}

static void lbzuTest()
{
  START_TEST();

  u8 values[256];
  for (int i = 0; i < 256; ++i)
    values[i] = i;

  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
  {
    u8 result = 0;
    u32 address = (u32)&values[i];
    asm("lbzu %0, 0(%1)" : "=r"(result), "+r"(address));
    DO_TEST(result == values[i], "lbzu({}):\n"
                                 "\tgot {}\n"
                                 "\texpected {}",
            i, result, values[i]);
    DO_TEST(address == (u32)&values[i], "lbzu({}):\n"
                                        "\tgot 0x{:08x}\n"
                                        "\texpected 0x{:08x}",
            i, address, (u32)&values[i]);
  }
  END_TEST();
}

static void lbzuxTest()
{
  START_TEST();

  u8 values[256];
  for (int i = 0; i < 256; ++i)
    values[i] = i;

  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
  {
    u8 result = 0;
    u32 address = (u32)&values[0];
    u32 offset = (u32)&values[i] - address;
    asm("lbzux %0, %1, %2" : "=r"(result), "+r"(offset) : "r"(address));
    DO_TEST(result == values[i], "lbzux({}):\n"
                                 "\tgot {}\n"
                                 "\texpected {}",
            i, result, values[i]);
    DO_TEST(offset == (u32)&values[i], "lbzux({}):\n"
                                       "\tgot 0x{:08x}\n"
                                       "\texpected 0x{:08x}",
            i, offset, (u32)&values[i]);
  }
  END_TEST();
}
int main()
{
  network_init();

  lwzTest();
  lwzuTest();
  lwzxTest();
  lwzuxTest();

  lhzTest();
  lhzxTest();
  lhzuTest();
  lhzuxTest();
  lhaTest();
  lhaxTest();
  lhauTest();
  lhauxTest();

  lbzTest();
  lbzxTest();
  lbzuTest();
  lbzuxTest();

  network_printf("Shutting down...\n");
  network_shutdown();

  return 0;
}
