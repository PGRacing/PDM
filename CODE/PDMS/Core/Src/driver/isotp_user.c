#include "isotp.h"
#include "main.h"
#include "can.h"
#include "typedefs.h"
#include "FreeRTOS.h"
#include "can_handler.h"

/* required, this must send a single CAN message with the given arbitration
    * ID (i.e. the CAN message ID) and data. The size will never be more than 8
    * bytes. Should return ISOTP_RET_OK if frame sent successfully.
    * May return ISOTP_RET_NOSPACE if the frame could not be sent but may be
    * retried later. Should return ISOTP_RET_ERROR in case frame could not be sent.
    */
int  isotp_user_send_can(const uint32_t arbitration_id,
                            const uint8_t* data, const uint8_t size) {

    T_CANH_TX_PACKAGE pkg = 
    {
        .header = 
        {
            .DLC = size,
            .ExtId = 0,
            .IDE = CAN_ID_STD,
            .RTR = CAN_RTR_DATA,
            .StdId = arbitration_id,
        }
    };

    memcpy((uint8_t*)&(pkg.data.raw), data, size);
    
    CANH_PushToTxQueue(CANH_INSTANCE_2, pkg);
    return ISOTP_RET_OK;
}

/* required, return system tick, unit is micro-second */
uint32_t isotp_user_get_us(void) {
    return xTaskGetTickCount() * 1000;
}

/* optional, provide to receive debugging log messages */
void isotp_user_debug(__unused const char* message, ...) {
    __NOP();
}