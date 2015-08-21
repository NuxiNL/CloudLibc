// Copyright (c) 2015 Nuxi, https://nuxi.nl/
//
// This file is distrbuted under a 2-clause BSD license.
// See the LICENSE file for details.

#include <sys/socket.h>

#include <netinet/in.h>

#include <netdb.h>
#include <stddef.h>
#include <testing.h>

TEST(getnameinfo, bad) {
  // Bad flags value.
  struct sockaddr_in sin = {.sin_family = AF_INET};
  ASSERT_EQ(EAI_BADFLAGS, getnameinfo((struct sockaddr *)&sin, sizeof(sin),
                                      NULL, 0, NULL, 0, 0xdeadc0de));

  // Unsupported family.
  struct sockaddr sa = {.sa_family = AF_UNSPEC};
  ASSERT_EQ(EAI_FAMILY, getnameinfo(&sa, sizeof(sa), (char *)42, 0, (char *)42,
                                    0, NI_NUMERICHOST | NI_NUMERICSCOPE));

  // Nothing to do: both node and service are NULL.
  ASSERT_EQ(EAI_NONAME, getnameinfo(NULL, 0, NULL, 0, NULL, 0, 0));
}

#define TEST_SOCKADDR(sa, flags, node, service)                                \
  do {                                                                         \
    /* Perform conversion. */                                                  \
    char nodebuf[sizeof(node)];                                                \
    char servicebuf[sizeof(service)];                                          \
    ASSERT_EQ(0, getnameinfo((struct sockaddr *) & (sa), sizeof(sa), nodebuf,  \
                             sizeof(nodebuf), servicebuf, sizeof(servicebuf),  \
                             NI_NUMERICHOST | (flags)));                       \
    ASSERT_STREQ(node, nodebuf);                                               \
    ASSERT_STREQ(service, servicebuf);                                         \
                                                                               \
    /* Test what happens if the output buffer is too small. */                 \
    ASSERT_EQ(EAI_OVERFLOW,                                                    \
              getnameinfo((struct sockaddr *) & (sa), sizeof(sa), nodebuf,     \
                          sizeof(nodebuf) - 1, servicebuf, sizeof(servicebuf), \
                          NI_NUMERICHOST | (flags)));                          \
    ASSERT_EQ(EAI_OVERFLOW,                                                    \
              getnameinfo((struct sockaddr *) & (sa), sizeof(sa), nodebuf,     \
                          sizeof(nodebuf), servicebuf, sizeof(servicebuf) - 1, \
                          NI_NUMERICHOST | (flags)));                          \
  } while (0)

TEST(getnameinfo, inet) {
#define TEST_INET(addr, port, flags, node, service)           \
  do {                                                        \
    /* Perform conversion. */                                 \
    struct sockaddr_in sin = {.sin_family = AF_INET,          \
                              .sin_addr.s_addr = htonl(addr), \
                              .sin_port = htons(port)};       \
    TEST_SOCKADDR(sin, flags, node, service);                 \
  } while (0)
  // TODO(ed): Add more tests!
  TEST_INET(0x00000000, 80, NI_NUMERICSERV, "0.0.0.0", "80");
  TEST_INET(0x0808b287, 22, NI_NUMERICSERV, "8.8.178.135", "22");
#undef TEST_INET
}

// TODO(ed): Test IPv6.