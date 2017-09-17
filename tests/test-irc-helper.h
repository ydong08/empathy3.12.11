#include <string.h>

#include <tp-account-widgets/tpaw-irc-network-manager.h>

#ifndef __CHECK_IRC_HELPER_H__
#define __CHECK_IRC_HELPER_H__

struct server_t
{
  gchar *address;
  guint port;
  gboolean ssl;
};

void check_server (TpawIrcServer *server, const gchar *_address,
    guint _port, gboolean _ssl);

void check_network (TpawIrcNetwork *network, const gchar *_name,
    const gchar *_charset, struct server_t *_servers, guint nb_servers);

#endif /* __CHECK_IRC_HELPER_H__ */
