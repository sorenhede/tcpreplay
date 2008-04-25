/* $Id$ */

/*
 * Copyright (c) 2006-2007 Aaron Turner.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright owners nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>

#include "dlt_plugins-int.h"
#include "dlt_utils.h"
#include "ieee80211.h"
#include "ieee80211_hdr.h"
#include "tcpedit.h"
#include "common.h"
#include "tcpr.h"

/*
 * Notes about the ieee80211 plugin:
 * 802.11 is a little different from most other L2 protocols:
 * - Not all frames are data frames (control, data, management)
 */
static char dlt_name[] = "ieee80211";
_U_ static char dlt_prefix[] = "ieee802_11";
static u_int16_t dlt_value = DLT_IEEE802_11;

/*
 * Function to register ourselves.  This function is always called, regardless
 * of what DLT types are being used, so it shouldn't be allocating extra buffers
 * or anything like that (use the dlt_ieee80211_init() function below for that).
 * Tasks:
 * - Create a new plugin struct
 * - Fill out the provides/requires bit masks.  Note:  Only specify which fields are
 *   actually in the header.
 * - Add the plugin to the context's plugin chain
 * Returns: TCPEDIT_ERROR | TCPEDIT_OK | TCPEDIT_WARN
 */
int 
dlt_ieee80211_register(tcpeditdlt_t *ctx)
{
    tcpeditdlt_plugin_t *plugin;
    assert(ctx);

    /* create  a new plugin structure */
    plugin = tcpedit_dlt_newplugin();

    /* we're a decoder only plugin */
    plugin->provides += PLUGIN_MASK_PROTO + PLUGIN_MASK_SRCADDR + PLUGIN_MASK_DSTADDR;
    plugin->requires += 0;
    
     /* what is our DLT value? */
    plugin->dlt = dlt_value;

    /* set the prefix name of our plugin.  This is also used as the prefix for our options */
    plugin->name = safe_strdup(dlt_name);

    /* 
     * Point to our functions, note, you need a function for EVERY method.  
     * Even if it is only an empty stub returning success.
     */
    plugin->plugin_init = dlt_ieee80211_init;
    plugin->plugin_cleanup = dlt_ieee80211_cleanup;
    plugin->plugin_parse_opts = dlt_ieee80211_parse_opts;
    plugin->plugin_decode = dlt_ieee80211_decode;
    plugin->plugin_encode = dlt_ieee80211_encode;
    plugin->plugin_proto = dlt_ieee80211_proto;
    plugin->plugin_l2addr_type = dlt_ieee80211_l2addr_type;
    plugin->plugin_l2len = dlt_ieee80211_l2len;
    plugin->plugin_get_layer3 = dlt_ieee80211_get_layer3;
    plugin->plugin_merge_layer3 = dlt_ieee80211_merge_layer3;
    plugin->plugin_get_mac = dlt_ieee80211_get_mac;

    /* add it to the available plugin list */
    return tcpedit_dlt_addplugin(ctx, plugin);
}

 
/*
 * Initializer function.  This function is called only once, if and only iif
 * this plugin will be utilized.  Remember, if you need to keep track of any state, 
 * store it in your plugin->config, not a global!
 * Returns: TCPEDIT_ERROR | TCPEDIT_OK | TCPEDIT_WARN
 */
int 
dlt_ieee80211_init(tcpeditdlt_t *ctx)
{
    tcpeditdlt_plugin_t *plugin;
    ieee80211_config_t *config;
    assert(ctx);
    
    if ((plugin = tcpedit_dlt_getplugin(ctx, dlt_value)) == NULL) {
        tcpedit_seterr(ctx->tcpedit, "Unable to initalize unregistered plugin %s", dlt_name);
        return TCPEDIT_ERROR;
    }
    
    /* allocate memory for our deocde extra data */
    if (sizeof(ieee80211_extra_t) > 0)
        ctx->decoded_extra = safe_malloc(sizeof(ieee80211_extra_t));

    /* allocate memory for our config data */
    if (sizeof(ieee80211_config_t) > 0)
        plugin->config = safe_malloc(sizeof(ieee80211_config_t));
    
    config = (ieee80211_config_t *)plugin->config;
    
    /* FIXME: set default config values here */

    return TCPEDIT_OK; /* success */
}

/*
 * Since this is used in a library, we should manually clean up after ourselves
 * Unless you allocated some memory in dlt_ieee80211_init(), this is just an stub.
 * Returns: TCPEDIT_ERROR | TCPEDIT_OK | TCPEDIT_WARN
 */
int 
dlt_ieee80211_cleanup(tcpeditdlt_t *ctx)
{
    tcpeditdlt_plugin_t *plugin;
    assert(ctx);

    if ((plugin = tcpedit_dlt_getplugin(ctx, dlt_value)) == NULL) {
        tcpedit_seterr(ctx->tcpedit, "Unable to cleanup unregistered plugin %s", dlt_name);
        return TCPEDIT_ERROR;
    }

    if (ctx->decoded_extra != NULL) {
        safe_free(ctx->decoded_extra);
        ctx->decoded_extra = NULL;
    }
        
    if (plugin->config != NULL) {
        safe_free(plugin->config);
        plugin->config = NULL;
    }

    return TCPEDIT_OK; /* success */
}

/*
 * This is where you should define all your AutoGen AutoOpts option parsing.
 * Any user specified option should have it's bit turned on in the 'provides'
 * bit mask.
 * Returns: TCPEDIT_ERROR | TCPEDIT_OK | TCPEDIT_WARN
 */
int 
dlt_ieee80211_parse_opts(tcpeditdlt_t *ctx)
{
    assert(ctx);

    /* we have none */
    
    return TCPEDIT_OK; /* success */
}

/*
 * Function to decode the layer 2 header in the packet.
 * You need to fill out:
 * - ctx->l2len
 * - ctx->srcaddr
 * - ctx->dstaddr
 * - ctx->proto
 * - ctx->decoded_extra
 * Returns: TCPEDIT_ERROR | TCPEDIT_OK | TCPEDIT_WARN
 */
int 
dlt_ieee80211_decode(tcpeditdlt_t *ctx, const u_char *packet, const int pktlen)
{
    assert(ctx);
    assert(packet);
    assert(pktlen > dlt_ieee80211_l2len(ctx, packet, pktlen));

    if (!ieee80211_is_data(ctx, packet, pktlen)) {
        tcpedit_seterr(ctx->tcpedit, "Packet " COUNTER_SPEC " is not a normal 802.11 data frame",
            ctx->tcpedit->runtime.packetnum);
        return TCPEDIT_SOFT_ERROR;
    }
    
    if (ieee80211_is_encrypted(ctx, packet, pktlen)) {
        tcpedit_seterr(ctx->tcpedit, "Packet " COUNTER_SPEC " is encrypted.  Unable to decode frame.",
            ctx->tcpedit->runtime.packetnum);
        return TCPEDIT_SOFT_ERROR;
    }

    memcpy(&(ctx->srcaddr), ieee80211_get_src((ieee80211_hdr_t *)packet), ETHER_ADDR_LEN);
    memcpy(&(ctx->dstaddr), ieee80211_get_dst((ieee80211_hdr_t *)packet), ETHER_ADDR_LEN);
    ctx->proto = dlt_ieee80211_proto(ctx, packet, pktlen);

    return TCPEDIT_OK; /* success */
}

/*
 * Function to encode the layer 2 header back into the packet.
 * Returns: total packet len or TCPEDIT_ERROR
 */
int 
dlt_ieee80211_encode(tcpeditdlt_t *ctx, u_char **packet_ex, int pktlen, _U_ tcpr_dir_t dir)
{
    u_char *packet;
    assert(ctx);
    assert(packet_ex);
    assert(pktlen);
    
    packet = *packet_ex;
    assert(packet);
    
    tcpedit_seterr(ctx->tcpedit, "%s", "DLT_IEEE802_11 plugin does not support packet encoding");
    return TCPEDIT_ERROR;
}

/*
 * Function returns the Layer 3 protocol type of the given packet, or TCPEDIT_ERROR on error
 */
int 
dlt_ieee80211_proto(tcpeditdlt_t *ctx, const u_char *packet, const int pktlen)
{
    int protocol, l2len;

    assert(ctx);
    assert(packet);

    l2len = dlt_ieee80211_l2len(ctx, packet, pktlen);

    assert(pktlen >= l2len);
    
    protocol = (u_int16_t)packet[l2len - 2];
    
    return protocol;
}

/*
 * Function returns a pointer to the layer 3 protocol header or NULL on error
 */
u_char *
dlt_ieee80211_get_layer3(tcpeditdlt_t *ctx, u_char *packet, const int pktlen)
{
    int l2len;
    assert(ctx);
    assert(packet);

    l2len = dlt_ieee80211_l2len(ctx, packet, pktlen);

    assert(pktlen >= l2len);
    
    return tcpedit_dlt_l3data_copy(ctx, packet, pktlen, l2len);
}

/*
 * function merges the packet (containing L2 and old L3) with the l3data buffer
 * containing the new l3 data.  Note, if L2 % 4 == 0, then they're pointing to the
 * same buffer, otherwise there was a memcpy involved on strictly aligned architectures
 * like SPARC
 */
u_char *
dlt_ieee80211_merge_layer3(tcpeditdlt_t *ctx, u_char *packet, const int pktlen, u_char *l3data)
{
    int l2len;
    assert(ctx);
    assert(packet);
    assert(l3data);
    
    l2len = dlt_ieee80211_l2len(ctx, packet, pktlen);
    
    assert(pktlen >= l2len);
    
    return tcpedit_dlt_l3data_merge(ctx, packet, pktlen, l3data, l2len);
}

/* 
 * return the length of the L2 header of the current packet
 * based on: http://www.tcpdump.org/lists/workers/2004/07/msg00121.html
 * Returns >= 0 or TCPEDIT_SOFT_ERROR on error
 *
 */
int
dlt_ieee80211_l2len(tcpeditdlt_t *ctx, const u_char *packet, const int pktlen)
{
    u_int16_t *frame_control, fc;
    struct tcpr_802_2snap_hdr *hdr;
    int hdrlen = 0;


    assert(ctx);
    assert(packet);
    assert(pktlen);
    

    frame_control = (u_int16_t *)packet;
    fc = ntohs(*frame_control);

    if (ieee80211_USE_4(fc)) {
        hdrlen = sizeof(ieee80211_addr4_hdr_t);
    } else {
        hdrlen = sizeof(ieee80211_hdr_t);
    }

    /* 
     * FIXME: 802.11e?  has a QoS feature which apparently extends the header by another
     * 2 bytes, but I don't know how to test for that yet.
     */
    
    if (pktlen < hdrlen + (int)sizeof(struct tcpr_802_2snap_hdr)) {
        return TCPEDIT_SOFT_ERROR;
    }
    hdr = (struct tcpr_802_2snap_hdr *)&packet[hdrlen];
    
    /* verify the header is 802.2SNAP (8 bytes) not 802.2 (3 bytes) */
    if (hdr->snap_dsap == 0xAA && hdr->snap_ssap == 0xAA) {
        hdrlen += (int)sizeof(struct tcpr_802_2snap_hdr);
    } else {
        hdrlen += (int)sizeof(struct tcpr_802_2_hdr);
    }

    return hdrlen;
}

/*
 * return a static pointer to the source/destination MAC address
 * return NULL on error/address doesn't exist
 */    
u_char *
dlt_ieee80211_get_mac(tcpeditdlt_t *ctx, tcpeditdlt_mac_type_t mac, const u_char *packet, const int pktlen)
{
    assert(ctx);
    assert(packet);
    assert(pktlen);
    char *macaddr;
    
    switch(mac) {
    case SRC_MAC:
        macaddr = ieee80211_get_src(packet);
        memcpy(ctx->srcmac, macaddr, ETHER_ADDR_LEN);
        return(ctx->srcmac);
        break;
        
    case DST_MAC:
        macaddr = ieee80211_get_dst(packet);
        memcpy(ctx->dstmac, macaddr, ETHER_ADDR_LEN);
        return(ctx->dstmac);
        break;
        
    default:
        errx(1, "Invalid tcpeditdlt_mac_type_t: %d", mac);
    }
    return(NULL);
}


tcpeditdlt_l2addr_type_t 
dlt_ieee80211_l2addr_type(void)
{
    return ETHERNET;
}
