// Copyright (c) 2015 Nuxi, https://nuxi.nl/
//
// This file is distributed under a 2-clause BSD license.
// See the LICENSE file for details.

#ifndef MQUEUE_MQUEUE_IMPL_H
#define MQUEUE_MQUEUE_IMPL_H

#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Message.
//
// Messages are stored in a linked list, sorted in the order in which
// they should be extracted by mq_receive(). As mq_send() has to use the
// message priority to determine where the message needs to be inserted,
// we make use of a skip list.
//
// The {queue,next}_receive pointers point to every individual message,
// whereas the {queue_next}_send pointers only point to the last message
// at every priority. This allows us to insert messages in O(p) time,
// where p is the number of different message priorities used.
//
// In most common setups, p tends to be pretty low.
struct message {
  struct message *next_receive;  // Next message to be returned.
  struct message *next_send;     // Last message of the next priority.
  size_t length;                 // Length of the message body.
  unsigned int priority;         // Priority of the message.
  char contents[];               // Message body.
};

// Message queue.
struct __mqd {
  pthread_mutex_t lock;           // Queue lock.
  pthread_cond_t cond;            // Queue condition variable for sleeps.
  struct mq_attr attr;            // Queue attributes.
  struct message *queue_receive;  // List of messages to be returned.
  struct message *queue_send;     // Last message of the highest priority.
};

static inline bool mq_receive_pre(mqd_t mqdes, size_t msg_len)
    __trylocks_exclusive(true, mqdes->lock) {
  // Fail if the provided buffer size is less than the message size
  // attribute of the message queue.
  pthread_mutex_lock(&mqdes->lock);
  if (msg_len < (size_t)mqdes->attr.mq_msgsize) {
    pthread_mutex_unlock(&mqdes->lock);
    errno = EMSGSIZE;
    return false;
  }

  // Fail if the queue has no messages and has the non-blocking flag set.
  if (mqdes->attr.mq_curmsgs <= 0 && (mqdes->attr.mq_flags & O_NONBLOCK) != 0) {
    pthread_mutex_unlock(&mqdes->lock);
    errno = EAGAIN;
    return false;
  }
  return true;
}

static inline size_t mq_receive_post(mqd_t mqdes, char *msg_ptr,
                                     unsigned int *msg_prio)
    __unlocks(mqdes->lock) {
  // Extract the oldest message from the queue.
  struct message *m = mqdes->queue_receive;
  mqdes->queue_receive = m->next_send;
  --mqdes->attr.mq_curmsgs;

  // If the message is the only message at that priority, update the
  // skip list to point to the next priority.
  if (mqdes->queue_send == m)
    mqdes->queue_send = m->next_send;
  pthread_mutex_unlock(&mqdes->lock);

  // Copy out the message contents and free it.
  size_t length = m->length;
  memcpy(msg_ptr, m->contents, length);
  if (msg_prio != NULL)
    *msg_prio = m->priority;
  free(m);
  return length;
}

static inline bool mq_send_pre(mqd_t mqdes, size_t msg_len)
    __trylocks_exclusive(true, mqdes->lock) {
  // Fail if the size of the provided message is more than the message
  // size attribute of the message queue.
  pthread_mutex_lock(&mqdes->lock);
  if (msg_len > (size_t)mqdes->attr.mq_msgsize) {
    pthread_mutex_unlock(&mqdes->lock);
    errno = EMSGSIZE;
    return false;
  }

  // Fail if the queue is full and has the non-blocking flag set.
  if (mqdes->attr.mq_curmsgs >= mqdes->attr.mq_maxmsg &&
      (mqdes->attr.mq_flags & O_NONBLOCK) != 0) {
    pthread_mutex_unlock(&mqdes->lock);
    errno = EAGAIN;
    return false;
  }
  return true;
}

static inline int mq_send_post(mqd_t mqdes, const char *msg_ptr, size_t msg_len,
                               unsigned int msg_prio) __unlocks(mqdes->lock) {
  // Create a message object and initialize it.
  struct message *m = malloc(offsetof(struct message, contents) + msg_len);
  if (m == NULL) {
    pthread_mutex_unlock(&mqdes->lock);
    return -1;
  }
  m->length = msg_len;
  memcpy(m->contents, msg_ptr, msg_len);

  // Scan through the list of messages to find the spot where the
  // message needs to be inserted.
  struct message **m_receive = &mqdes->queue_receive;
  struct message **m_send = &mqdes->queue_send;
  while (*m_send != NULL) {
    if ((*m_send)->priority <= msg_prio) {
      if ((*m_send)->priority == msg_prio) {
        // We already have other messages enqueued at the same priority.
        // Place the message behind the currently last message at that
        // priority and update the skip list to point to the new message
        // instead.
        m->next_receive = (*m_send)->next_receive;
        m->next_send = (*m_send)->next_send;
        (*m_send)->next_receive = m;
        *m_send = m;
        goto inserted;
      }
      break;
    } else {
      m_receive = &(*m_send)->next_receive;
      m_send = &(*m_send)->next_send;
    }
  }

  // First message at this priority. Insert the message into both the
  // receive and send queue.
  m->next_receive = *m_receive;
  *m_receive = m;
  m->next_send = *m_send;
  *m_send = m;

inserted:
  // Successfully inserted the message into the queue.
  ++mqdes->attr.mq_curmsgs;
  pthread_mutex_unlock(&mqdes->lock);
  return 0;
}

#endif
