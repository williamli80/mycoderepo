#include "globalhandler.h"
#include "ipmid-api.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

const char  *control_object_name  =  "/org/openbmc/control/bmc0";
const char  *control_intf_name    =  "org.openbmc.control.Bmc";

const char  *objectmapper_service_name =  "org.openbmc.objectmapper";
const char  *objectmapper_object_name  =  "/org/openbmc/objectmapper/objectmapper";
const char  *objectmapper_intf_name    =  "org.openbmc.objectmapper.ObjectMapper";

void register_netfn_global_functions() __attribute__((constructor));

int obj_mapper_get_connection(char** buf, const char* obj_path)
{
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = NULL;
    sd_bus *bus = NULL;
    char *temp_buf = NULL, *intf = NULL;
    size_t buf_size = 0;
    int r;

    // Open the system bus where most system services are provided.
    r = sd_bus_open_system(&bus);
    if (r < 0) {
        fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
        goto finish;
    }

    // Bus, service, object path, interface and method are provided to call
    // the method.
    // Signatures and input arguments are provided by the arguments at the
    // end.
    r = sd_bus_call_method(bus,
            objectmapper_service_name,                      /* service to contact */
            objectmapper_object_name,                       /* object path */
            objectmapper_intf_name,                         /* interface name */
            "GetObject",                                 /* method name */
            &error,                                      /* object to return error in */
            &m,                                          /* return message on success */
            "s",                                         /* input signature */
            obj_path                                     /* first argument */
            );

    if (r < 0) {
        fprintf(stderr, "Failed to issue method call: %s\n", error.message);
        goto finish;
    }

    // Get the key, aka, the connection name
    sd_bus_message_read(m, "a{sas}", 1, &temp_buf, 1, &intf);
    // TODO: check the return code. Currently for no reason the message
    // parsing of object mapper is always complaining about
    // "Device or resource busy", but the result seems OK for now. Need
    // further checks.
    //r = sd_bus_message_read(m, "a{sas}", 1, &temp_buf, 1, &intf);
    //if (r < 0) {
    //    fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
    //    goto finish;
    //}

    buf_size = strlen(temp_buf) + 1;
    printf("IPMID connection name: %s\n", temp_buf);
    *buf = (char*)malloc(buf_size);

    if (*buf == NULL) {
        fprintf(stderr, "Malloc failed for get_sys_boot_options");
        r = -1;
        goto finish;
    }

    memcpy(*buf, temp_buf, buf_size);

finish:
    sd_bus_error_free(&error);
    sd_bus_message_unref(m);
    sd_bus_unref(bus);

    return r;
}

int dbus_cold_reset()
{
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = NULL;
    sd_bus *bus = NULL;
    char* temp_buf = NULL;
    uint8_t* get_value = NULL;
    char* connection = NULL;
    int r, i;

    r = obj_mapper_get_connection(&connection, control_object_name);
    if (r < 0) {
        fprintf(stderr, "Failed to get connection, return value: %d.\n", r);
        goto finish;
    }

    printf("connection: %s\n", connection);

    // Open the system bus where most system services are provided.
    r = sd_bus_open_system(&bus);
    if (r < 0) {
        fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
        goto finish;
    }

    // Bus, service, object path, interface and method are provided to call
    // the method.
    // Signatures and input arguments are provided by the arguments at the
    // end.
    // TODO: the method is a place holder, not cold reset, need to call the correct
    // method.
    r = sd_bus_call_method(bus,
            connection,                                /* service to contact */
            control_object_name,                       /* object path */
            control_intf_name,                         /* interface name */
            "place_holder",                            /* method name */
            &error,                                    /* object to return error in */
            &m,                                        /* return message on success */
            NULL,
            NULL
            );

    if (r < 0) {
        fprintf(stderr, "Failed to issue method call: %s\n", error.message);
        goto finish;
    }

finish:
    sd_bus_error_free(&error);
    sd_bus_message_unref(m);
    sd_bus_unref(bus);
    free(connection);

    return r;
}

ipmi_ret_t ipmi_global_cold_reset(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                              ipmi_request_t request, ipmi_response_t response,
                              ipmi_data_len_t data_len, ipmi_context_t context)
{
    printf("Handling GLOBAL COLD RESET Netfn:[0x%X], Cmd:[0x%X]\n",netfn, cmd);

    // TODO: call the correct dbus method for cold reset.
    // Currently the reboot method called is just a placeholder. 
    dbus_cold_reset();

    // Status code.
    ipmi_ret_t rc = IPMI_CC_OK;
    *data_len = 0;
    return rc;
}

void register_netfn_global_functions()
{
    printf("Registering NetFn:[0x%X], Cmd:[0x%X]\n",NETFUN_APP, IPMI_CMD_COLD_RESET);
    ipmi_register_callback(NETFUN_APP, IPMI_CMD_COLD_RESET, NULL, ipmi_global_cold_reset);
}
