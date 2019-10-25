/*
 * librdkafka - Apache Kafka C library
 *
 * Copyright (c) 2019 Magnus Edenhill
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _RDKAFKA_MOCK_H_
#define _RDKAFKA_MOCK_H_


/**
 * @struct A real TCP connection from the client to a mock broker.
 */
typedef struct rd_kafka_mock_connection_s {
        TAILQ_ENTRY(rd_kafka_mock_connection_s) link;
        rd_kafka_transport_t *transport; /**< Socket transport */
        rd_kafka_buf_t *rxbuf;   /**< Receive buffer */
        rd_kafka_bufq_t outbufs; /**< Send buffers */
        short *poll_events;      /**< Events to poll, points to
                                  *   the broker's pfd array */
        struct sockaddr_in peer; /**< Peer address */
        struct rd_kafka_mock_broker_s *broker;
} rd_kafka_mock_connection_t;


/**
 * @struct Mock broker
 */
typedef struct rd_kafka_mock_broker_s {
        TAILQ_ENTRY(rd_kafka_mock_broker_s) link;
        int32_t id;
        char    advertised_listener[128];
        int     port;
        char   *rack;

        int     listen_s;   /**< listen() socket */

        TAILQ_HEAD(, rd_kafka_mock_connection_s) connections;

        struct rd_kafka_mock_cluster_s *cluster;
} rd_kafka_mock_broker_t;


/**
 * @struct Mock partition
 */
typedef struct rd_kafka_mock_partition_s {
        TAILQ_ENTRY(rd_kafka_mock_partition_s) leader_link;
        int32_t id;

        rd_kafka_mock_broker_t  *leader;
        rd_kafka_mock_broker_t **replicas;
        int                      replica_cnt;

        struct rd_kafka_mock_topic_s *topic;
} rd_kafka_mock_partition_t;


/**
 * @struct Mock topic
 */
typedef struct rd_kafka_mock_topic_s {
        TAILQ_ENTRY(rd_kafka_mock_topic_s) link;
        char   *name;

        rd_kafka_mock_partition_t *partitions;
        int     partition_cnt;

        struct rd_kafka_mock_cluster_s *cluster;
} rd_kafka_mock_topic_t;


typedef void (rd_kafka_mock_io_handler_t) (struct rd_kafka_mock_cluster_s
                                           *mcluster,
                                           int fd, int events, void *opaque);

/**
 * @struct Mock cluster.
 *
 * The cluster IO loop runs in a separate thread where all
 * broker IO is handled.
 *
 * No locking is needed.
 */
typedef struct rd_kafka_mock_cluster_s {
        char id[32];  /**< Generated cluster id */

        rd_kafka_t *rk;

        int32_t controller_id;  /**< Current controller */

        TAILQ_HEAD(, rd_kafka_mock_broker_s) brokers;
        int broker_cnt;

        TAILQ_HEAD(, rd_kafka_mock_topic_s) topics;
        int topic_cnt;

        char *bootstraps; /**< bootstrap.servers */

        thrd_t thread;    /**< Mock thread */
        int ctrl_s[2];    /**< Control socket for terminating the mock thread.
                           *   [0] is the thread's read socket,
                           *   [1] is the rdkafka main thread's write socket.
                           */


        rd_bool_t run;    /**< Cluster will run while this value is true */

        int                         fd_cnt;   /**< Number of file descriptors */
        int                         fd_size;  /**< Allocated size of .fds
                                               *   and .handlers */
        struct pollfd              *fds;      /**< Dynamic array */

        rd_kafka_broker_t *dummy_rkb;  /**< Some internal librdkafka APIs
                                        *   that we are reusing requires a
                                        *   broker object, we use the
                                        *   internal broker and store it
                                        *   here for convenient access. */

        struct {
                int partition_cnt;      /**< Auto topic create part cnt */
                int replication_factor; /**< Auto topic create repl factor */
        } defaults;

        /**< Dynamic array of IO handlers for corresponding fd in .fds */
        struct {
                rd_kafka_mock_io_handler_t *cb; /**< Callback */
                void *opaque;                   /**< Callbacks' opaque */
        } *handlers;
} rd_kafka_mock_cluster_t;



void rd_kafka_mock_broker_set_rack (rd_kafka_mock_cluster_t *mcluster,
                                    int32_t broker_id, const char *rack);
void rd_kafka_mock_cluster_destroy (rd_kafka_mock_cluster_t *mcluster);
rd_kafka_mock_cluster_t *rd_kafka_mock_cluster_new (rd_kafka_t *rk,
                                                    int broker_cnt);
const char *
rd_kafka_mock_cluster_bootstraps (const rd_kafka_mock_cluster_t *mcluster);

#endif /* _RDKAFKA_MOCK_H_ */
