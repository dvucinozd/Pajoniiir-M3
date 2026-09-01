#include "control_link.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

static bool s_wake_active;
static unsigned s_activity_calls;

static bool activity_cb(void)
{
    s_activity_calls++;
    return s_wake_active;
}

static ctrl_event_t receive_one(QueueHandle_t queue)
{
    ctrl_event_t event = {0};
    assert(xQueueReceive(queue, &event, 0u) == pdTRUE);
    return event;
}

static void test_local_wake_event_is_consumed(void)
{
    QueueHandle_t queue = xQueueCreate(4u, sizeof(ctrl_event_t));
    assert(queue != NULL);
    assert(control_link_init(queue) == ESP_OK);
    control_link_set_activity_cb(activity_cb);
    s_activity_calls = 0u;
    s_wake_active = true;

    assert(control_link_inject_semantic(CTRL_TYPE_BUTTON,
                                        CTRL_ID_DECK1_PLAY,
                                        1) == ESP_OK);
    assert(s_activity_calls == 1u);
    assert(uxQueueMessagesWaiting(queue) == 0u);
    vQueueDelete(queue);
}

static void test_awake_local_event_is_queued(void)
{
    QueueHandle_t queue = xQueueCreate(4u, sizeof(ctrl_event_t));
    assert(queue != NULL);
    assert(control_link_init(queue) == ESP_OK);
    control_link_set_activity_cb(activity_cb);
    s_activity_calls = 0u;
    s_wake_active = false;

    assert(control_link_inject_semantic(CTRL_TYPE_BUTTON,
                                        CTRL_ID_DECK1_PLAY,
                                        1) == ESP_OK);
    assert(s_activity_calls == 1u);
    ctrl_event_t event = receive_one(queue);
    assert(event.type == CTRL_EV_BUTTON);
    assert(event.id == CTRL_ID_DECK1_PLAY);
    assert(event.value == 1);
    vQueueDelete(queue);
}

static void test_connection_state_bypasses_activity_hook(void)
{
    QueueHandle_t queue = xQueueCreate(4u, sizeof(ctrl_event_t));
    assert(queue != NULL);
    assert(control_link_init(queue) == ESP_OK);
    control_link_set_activity_cb(activity_cb);
    s_activity_calls = 0u;
    s_wake_active = true;

    assert(control_link_inject_semantic(CTRL_TYPE_STATE,
                                        CTRL_ID_FLX4_CONNECTION,
                                        CTRL_FLX4_CONNECTED) == ESP_OK);
    assert(s_activity_calls == 0u);
    ctrl_event_t event = receive_one(queue);
    assert(event.type == CTRL_EV_STATE);
    assert(event.id == CTRL_ID_FLX4_CONNECTION);
    assert(event.value == CTRL_FLX4_CONNECTED);
    vQueueDelete(queue);
}

static void test_invalid_event_does_not_report_activity(void)
{
    QueueHandle_t queue = xQueueCreate(4u, sizeof(ctrl_event_t));
    assert(queue != NULL);
    assert(control_link_init(queue) == ESP_OK);
    control_link_set_activity_cb(activity_cb);
    s_activity_calls = 0u;
    s_wake_active = true;

    assert(control_link_inject_semantic(CTRL_TYPE_ENCODER, 0xFEu, 1) ==
           ESP_ERR_INVALID_ARG);
    assert(s_activity_calls == 0u);
    assert(uxQueueMessagesWaiting(queue) == 0u);
    vQueueDelete(queue);
}

int main(void)
{
    test_local_wake_event_is_consumed();
    test_awake_local_event_is_queued();
    test_connection_state_bypasses_activity_hook();
    test_invalid_event_does_not_report_activity();
    control_link_set_activity_cb(NULL);
    puts("control_link local activity tests passed");
    return 0;
}
