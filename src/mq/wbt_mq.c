/* 
 * File:   wbt_mq.c
 * Author: fcten
 *
 * Created on 2016年1月14日, 上午10:59
 */

#include "wbt_mq.h"
#include "wbt_mq_channel.h"
#include "wbt_mq_msg.h"
#include "wbt_mq_subscriber.h"
#include "wbt_mq_status.h"
#include "../json/wbt_json.h"

wbt_str_t wbt_str_message = wbt_string("message");
wbt_str_t wbt_str_channel = wbt_string("channel");
wbt_str_t wbt_str_subscriber = wbt_string("subscriber");
wbt_str_t wbt_str_total = wbt_string("total");
wbt_str_t wbt_str_active = wbt_string("active");
wbt_str_t wbt_str_delayed = wbt_string("delayed");
wbt_str_t wbt_str_waiting_ack = wbt_string("waiting_ack");
wbt_str_t wbt_str_system = wbt_string("system");
wbt_str_t wbt_str_uptime = wbt_string("uptime");
wbt_str_t wbt_str_channel_id = wbt_string("channel_id");
wbt_str_t wbt_str_list = wbt_string("list");

wbt_module_t wbt_module_mq = {
    wbt_string("mq"),
    wbt_mq_init, // init
    NULL, // exit
    NULL, // on_conn
    wbt_mq_on_recv, // on_recv
    NULL, // on_send
    wbt_mq_on_close,  // on_close
    wbt_mq_on_success
};

wbt_status wbt_mq_init() {
    wbt_mq_channel_init();
    wbt_mq_subscriber_init();
    wbt_mq_msg_init();
    
    wbt_mq_uptime();

    return WBT_OK;
}

time_t wbt_mq_uptime() {
    static time_t start_time = 0;

    if(!start_time) {
        start_time = wbt_cur_mtime;
    }
    
    return (wbt_cur_mtime - start_time)/1000;
}

wbt_status wbt_mq_on_recv(wbt_event_t *ev) {
    // 分发请求
    wbt_http_t * http = ev->data;

    // 只过滤 404 响应
    if( http->status != STATUS_404 ) {
        return WBT_OK;
    }

    wbt_str_t login  = wbt_string("/mq/login/");
    wbt_str_t pull   = wbt_string("/mq/pull/");
    wbt_str_t push   = wbt_string("/mq/push/");
    wbt_str_t status = wbt_string("/mq/status/");
    
    wbt_str_t http_uri;
    wbt_offset_to_str(http->uri, http_uri, ev->buff);
    
    if( wbt_strcmp( &http_uri, &login ) == 0 ) {
        return wbt_mq_login(ev);
    } else if( wbt_strcmp( &http_uri, &pull ) == 0 ) {
        return wbt_mq_pull(ev);
    } else if( wbt_strcmp( &http_uri, &push ) == 0 ) {
        return wbt_mq_push(ev);
    } else if( wbt_strncmp( &http_uri, &status, status.len ) == 0 ) {
        return wbt_mq_status(ev);
    }

    return WBT_OK;
}

wbt_status wbt_mq_on_close(wbt_event_t *ev) {
    wbt_subscriber_t *subscriber = ev->ctx;
    if( subscriber == NULL ) {
        return WBT_OK;
    }

    // 遍历所有订阅的频道
    wbt_channel_list_t *channel_node;
    wbt_list_for_each_entry(channel_node, &subscriber->channel_list->head, head) {
        // 从该频道的 subscriber_list 中移除该订阅者
        wbt_mq_channel_del_subscriber(channel_node->channel, subscriber);
    }

    // 重新投递发送失败的负载均衡消息
    wbt_msg_t *msg = wbt_mq_msg_get(subscriber->msg_id);
    if( msg && msg->delivery_mode == MSG_LOAD_BALANCE ) {
        wbt_mq_msg_delivery(msg);
    }

    wbt_msg_list_t *msg_node;
    // 遍历所有未投递的消息，重新投递负载均衡消息
    wbt_list_for_each_entry(msg_node, &subscriber->msg_list->head, head) {
        msg = wbt_mq_msg_get(msg_node->msg_id);
        if( msg && msg->delivery_mode == MSG_LOAD_BALANCE ) {
            wbt_mq_msg_delivery(msg);
        }
    }
    // 重新投递尚未返回 ACK 响应的负载均衡消息
    wbt_list_for_each_entry(msg_node, &subscriber->delivered_list->head, head) {
        msg = wbt_mq_msg_get(msg_node->msg_id);
        if( msg ) {
            wbt_mq_msg_delivery(msg);
        }
    }

    // 删除该订阅者
    wbt_mq_subscriber_destory(subscriber);
    
    return WBT_OK;
}

wbt_status wbt_mq_on_success(wbt_event_t *ev) {
    wbt_subscriber_t *subscriber = ev->ctx;
    if( subscriber == NULL || subscriber->msg_id == 0 ) {
        return WBT_OK;
    }
    
    wbt_msg_t *msg = wbt_mq_msg_get(subscriber->msg_id);
    if( msg ) {
        // 如果是负载均衡消息，将该消息移动到 delivered_list 中
        if( msg->delivery_mode == MSG_LOAD_BALANCE ) {
            wbt_msg_list_t *msg_node = wbt_mq_msg_create_node(subscriber->msg_id);
            if( msg_node == NULL ) {
                return WBT_ERROR;
            }
            wbt_list_add_tail( &msg_node->head, &subscriber->delivered_list->head );
        }
        
        wbt_mq_msg_inc_delivery(msg);
        subscriber->msg_id = 0;
    }
    
    return WBT_OK;
}

wbt_status wbt_mq_login(wbt_event_t *ev) {
    // 解析请求
    wbt_http_t * http = ev->data;

    // 必须是 POST 请求
    if( http->method != METHOD_POST ) {
        http->status = STATUS_405;
        return WBT_OK;
    }
    
    // 处理 body
    if( http->body.len <= 0 ) {
        http->status = STATUS_403;
        return WBT_OK;
    }
    
    // 检查是否已经登录过
    if( ev->ctx != NULL ) {
        wbt_mq_on_close(ev);
    }

    // 创建一个新的订阅者并初始化
    wbt_subscriber_t * subscriber = wbt_mq_subscriber_create();
    if( subscriber == NULL ) {
        // 返回登录失败
        return WBT_OK;
    }
    
    subscriber->ev = ev;
    ev->ctx = subscriber;
    
    wbt_log_debug("new subscriber %lld fron conn %d\n", subscriber->subscriber_id, ev->fd);
    
    // 在所有想要订阅的频道的 subscriber_list 中添加该订阅者
    wbt_str_t channel_ids;
    wbt_offset_to_str(http->body, channel_ids, ev->buff);
    wbt_mq_id channel_id = wbt_str_to_ull(&channel_ids, 10);

    // 遍历想要订阅的所有频道
        wbt_channel_t * channel = wbt_mq_channel_get(channel_id);
        if( channel == NULL ) {
            http->status = STATUS_503;
            return WBT_OK;
        }

        // 在该订阅者中添加一个频道 & 在该频道中添加一个订阅者
        if( wbt_mq_subscriber_add_channel(subscriber, channel) != WBT_OK ||
            wbt_mq_channel_add_subscriber(channel, subscriber) != WBT_OK ) {
            http->status = STATUS_503;
            return WBT_OK;
        }

        // 遍历该频道的 msg_list
        if( !wbt_list_empty(&channel->msg_list->head) ) {
            wbt_msg_list_t *msg_node, *next_node = wbt_list_next_entry(channel->msg_list, head);
            wbt_msg_t *msg;
            do {
                msg_node = next_node;
                next_node = wbt_list_next_entry(next_node, head);
                
                msg = wbt_mq_msg_get(msg_node->msg_id);
                if( !msg || msg->expire <= wbt_cur_mtime ) {
                    wbt_mq_channel_del_msg(channel, msg_node);
                    continue;
                }

                // 复制该消息到订阅者的 msg_list 中
                wbt_msg_list_t *tmp_node = wbt_mq_msg_create_node(msg_node->msg_id);
                if( tmp_node == NULL ) {
                    // 内存不足，操作失败
                    continue;
                }
                wbt_list_add_tail(&tmp_node->head, &subscriber->msg_list->head);

                // 如果是负载均衡消息，则从 msg_list 中移除该消息
                if( msg->delivery_mode == MSG_LOAD_BALANCE ) {
                    wbt_mq_channel_del_msg(channel, msg_node);
                }
            } while(next_node != channel->msg_list);
        }

    http->status = STATUS_200;
        
    return WBT_OK;
}

wbt_str_t wbt_mq_str_consumer_id   = wbt_string("consumer_id");
wbt_str_t wbt_mq_str_effect        = wbt_string("effect");
wbt_str_t wbt_mq_str_expire        = wbt_string("expire");
wbt_str_t wbt_mq_str_delivery_mode = wbt_string("delivery_mode");
wbt_str_t wbt_mq_str_data          = wbt_string("data");

wbt_status wbt_mq_parser( json_task_t * task, wbt_msg_t * msg ) {
    json_object_t * node = task->root;
    wbt_str_t key;
    
    while( node && node->object_type == JSON_OBJECT ) {
        key.str = node->key;
        key.len = node->key_len;
        switch( node->value_type ) {
            case JSON_LONGLONG:
                if ( wbt_strcmp(&key, &wbt_mq_str_consumer_id) == 0 ) {
                    msg->consumer_id = node->value.l;
                } else if ( wbt_strcmp(&key, &wbt_mq_str_effect) == 0 ) {
                    if( node->value.l >= 0 && node->value.l <= 2592000 ) {
                        msg->effect = (unsigned int)node->value.l;
                    }
                } else if ( wbt_strcmp(&key, &wbt_mq_str_expire) == 0 ) {
                    if( node->value.l >= 0 && node->value.l <= 2592000 ) {
                        msg->expire = (unsigned int)node->value.l;
                    }
                } else if ( wbt_strcmp(&key, &wbt_mq_str_delivery_mode) == 0 ) {
                    if(node->value.l == MSG_BROADCAST ) {
                        msg->delivery_mode = MSG_BROADCAST;
                    } else if ( node->value.l == MSG_LOAD_BALANCE ) {
                        msg->delivery_mode = MSG_LOAD_BALANCE;
                    } else {
                        return WBT_ERROR;
                    }
                }
                break;
            case JSON_STRING:
                if ( wbt_strcmp(&key, &wbt_mq_str_data) == 0 ) {
                    msg->data = wbt_strdup( node->value.s, node->value_len );
                    if( msg->data == NULL ) {
                        return WBT_ERROR;
                    }
                    msg->data_len = node->value_len;
                }
                break;
            case JSON_ARRAY:
                
                break;
            case JSON_OBJECT:
                if ( wbt_strcmp(&key, &wbt_mq_str_data) == 0 ) {
                    msg->data_len = 10240;
                    msg->data = wbt_malloc( msg->data_len );
                    if( msg->data == NULL ) {
                        return WBT_ERROR;
                    }
                    char *p = msg->data;
                    size_t l = msg->data_len;
                    json_print(node->value.p, &p, &l);
                    msg->data = wbt_realloc( msg->data, msg->data_len-l );
                }
                break;
        }

        node = node->next;
    }

    if( !msg->consumer_id || !msg->data_len ) {
        return WBT_ERROR;
    }

    msg->effect = msg->create + msg->effect * 1000;
    msg->expire = msg->effect + msg->expire * 1000;

    return WBT_OK;
}

wbt_status wbt_mq_push(wbt_event_t *ev) {
    // 解析请求
    wbt_http_t * http = ev->data;

    // 必须是 POST 请求
    if( http->method != METHOD_POST ) {
        http->status = STATUS_405;
        return WBT_OK;
    }
    
    // 解析请求
    wbt_str_t data;
    wbt_offset_to_str(http->body, data, ev->buff);

    json_task_t t;
    t.str = data.str;
    t.len = data.len;
    t.callback = NULL;

    if( json_parser(&t) != 0 ) {
        http->status = STATUS_403;
        return WBT_OK;
    }

    // 创建消息并初始化
    wbt_msg_t * msg = wbt_mq_msg_create();
    if( msg == NULL ) {
        // 返回投递失败
        json_delete_object(t.root);

        http->status = STATUS_503;
        return WBT_OK;
    }
    
    if( wbt_mq_parser(&t, msg) != WBT_OK ) {
        json_delete_object(t.root);
        wbt_mq_msg_destory( msg );

        http->status = STATUS_403;
        return WBT_OK;
    }
    
    json_delete_object(t.root);

    // TODO 持久化该消息
    
    // 投递消息
    if( wbt_mq_msg_delivery( msg ) != WBT_OK ) {
        wbt_mq_msg_destory( msg );
        
        http->status = STATUS_403;
        return WBT_OK;
    }
    
    // TODO 返回消息编号
    http->status = STATUS_200;

    return WBT_OK;
}

wbt_status wbt_mq_pull_timeout(wbt_event_t *ev) {
    // 固定返回一个空的响应，通知客户端重新发起 pull 请求
    wbt_http_t * http = ev->data;
    
    http->state = STATE_SENDING;
    http->status = STATUS_204;

    if( wbt_http_process(ev) != WBT_OK ) {
        wbt_conn_close(ev);
    } else {
        /* 等待socket可写 */
        ev->on_timeout = wbt_conn_close;
        ev->on_send = wbt_on_send;
        ev->events = EPOLLOUT | EPOLLET;
        ev->timeout = wbt_cur_mtime + wbt_conf.event_timeout;

        if(wbt_event_mod(ev) != WBT_OK) {
            return WBT_ERROR;
        }
    }

    return WBT_OK;
}

wbt_status wbt_mq_pull(wbt_event_t *ev) {
    wbt_http_t * http = ev->data;
    wbt_subscriber_t *subscriber = ev->ctx;
    
    if( ev->ctx == NULL ) {
        http->status = STATUS_404;
        return WBT_OK;
    }
    
    wbt_log_debug("subscriber %lld pull fron conn %d\n", subscriber->subscriber_id, ev->fd);

    
    // 从订阅者的 msg_list 中取出第一条消息
    while( !wbt_list_empty( &subscriber->msg_list->head ) ) {
        wbt_msg_list_t *msg_node = wbt_list_first_entry( &subscriber->msg_list->head, wbt_msg_list_t, head );
        wbt_msg_t *msg = wbt_mq_msg_get(msg_node->msg_id);
        
        // 如果消息已过期，则忽略
        if( !msg || msg->expire <= wbt_cur_mtime ) {
            // 从 msg_list 中删除该消息
            wbt_list_del( &msg_node->head );
            wbt_mq_msg_destory_node(msg_node);
            continue;
        }

        http->resp_body_memory.str = wbt_strdup(msg->data, msg->data_len);
        if( http->resp_body_memory.str == NULL ) {
            continue;
        }
        http->status = STATUS_200;
        http->resp_body_memory.len = msg->data_len;

        // 从 msg_list 中删除该消息
        wbt_list_del( &msg_node->head );
        wbt_mq_msg_destory_node(msg_node);
        
        // 保存当前正在处理的消息指针
        subscriber->msg_id = msg->msg_id;

        return WBT_OK;
    }
    
    if(1) {
        // 如果没有可发送的消息，挂起请求
        http->state = STATE_BLOCKING;

        ev->timeout = wbt_cur_mtime + 30000;
        ev->on_timeout = wbt_mq_pull_timeout;

        if(wbt_event_mod(ev) != WBT_OK) {
            return WBT_ERROR;
        }

        return WBT_OK;
    } else {
        // 如果没有可发送的消息，直接返回
        return wbt_mq_pull_timeout(ev);
    }
}

wbt_status wbt_mq_ack(wbt_event_t *ev) {
    // 解析请求
    wbt_mq_id msg_id;
    wbt_mq_id subscriber_id;
    
    // 该条消息消费成功
    
    // 从该订阅者的 delivered_heap 中移除消息
    
    return WBT_OK;
}

