/* ====================================================================
 * Copyright (c) 1996-1999 The Apache Group.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "networkio.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_network_io.h"
#include "apr_lib.h"
#include <sys/time.h>

ap_status_t ap_send(struct socket_t *sock, const char *buf, ap_ssize_t *len)
{
    ssize_t rv;
    
    do {
        rv = write(sock->socketdes, buf, (*len));
    } while (rv == -1 && errno == EINTR);

    if (rv == -1 && errno == EAGAIN && sock->timeout > 0) {
        int fds;
        int srv;

        do {
            fds = sock->socketdes;
            srv = os2_select(&fds, 0, 1, 0, sock->timeout);
        } while (srv == -1 && errno == EINTR);

        if (srv == 0) {
            (*len) = -1;
            return APR_TIMEUP;
        }
        else if (srv < 0) {
            (*len) = -1;
            return errno;
        }
        else {
            do {
                rv = write(sock->socketdes, buf, (*len));
            } while (rv == -1 && errno == EINTR);
        }
    }

    if (rv < 0) {
        *len = 0;
        return errno;
    }

    (*len) = rv;
    return APR_SUCCESS;
}



ap_status_t ap_recv(struct socket_t *sock, char *buf, ap_ssize_t *len)
{
    ssize_t rv;
    
    do {
        rv = read(sock->socketdes, buf, (*len));
    } while (rv == -1 && errno == EINTR);

    if (rv == -1 && errno == EAGAIN && sock->timeout > 0) {
        int fds;
        int srv;

        do {
            fds = sock->socketdes;
            srv = os2_select(&fds, 1, 0, 0, sock->timeout);
        } while (srv == -1 && errno == EINTR);

        if (srv == 0) {
            (*len) = -1;
            return APR_TIMEUP;
        }
        else if (srv < 0) {
            (*len) = -1;
            return errno;
        }
        else {
            do {
                rv = read(sock->socketdes, buf, (*len));
            } while (rv == -1 && errno == EINTR);
        }
    }

    if (rv < 0) {
        *len = 0;
        return errno;
    }

    (*len) = rv;
    return APR_SUCCESS;
}



ap_status_t ap_sendv(ap_socket_t *sock, const struct iovec *vec, ap_int32_t nvec, ap_int32_t *nbytes)
{
    ap_status_t rv;
    struct iovec *tmpvec;

    tmpvec = alloca(sizeof(struct iovec) * nvec);
    memcpy(tmpvec, vec, sizeof(struct iovec) * nvec);

    do {
        rv = writev(sock->socketdes, tmpvec, nvec);
    } while (rv == -1 && sock_errno() == SOCEINTR);

    if (rv == -1 && sock_errno() == SOCEWOULDBLOCK && sock->timeout != 0) {
        int fds;
        int srv;

        do {
            fds = sock->socketdes;
            srv = select(&fds, 1, 0, 0, sock->timeout >= 0 ? sock->timeout*1000 : -1);
        } while (srv == -1 && sock_errno() == SOCEINTR);

        if (srv == 0) {
            return APR_TIMEUP;
        }
        else if (srv < 0) {
            return os2errno(sock_errno());
        }
        else {
            do {
                rv = writev(sock->socketdes, (struct iovec *)vec, nvec);
            } while (rv == -1 && sock_errno() == SOCEINTR);
        }
    }
    
    if (rv < 0) {
        *nbytes = 0;
        return os2errno(sock_errno());
    }

    *nbytes = rv;
    return APR_SUCCESS;
}
