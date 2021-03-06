#ifdef __XYZZY__
#include "ed.h"
#endif
#include "sock.h"
#include "resolver.h"

resolver sock::s_resolver;

#ifdef __XYZZY__
static int WINAPI
blocking_hook ()
{
  Fdo_events ();
  if (QUITP)
    WSACancelBlockingCall ();
  return 0;
}
#endif

sock_error::sock_error (const char *ope)
     : e_error (WSAGetLastError ()), e_ope (ope)
{
}

const TCHAR *
sock::errmsg (int e)
{
  static const struct {int e; const TCHAR *s;} msg[] =
    {
      {WSAEINTR,           _T("Interrupted system call")},
      {WSAEBADF,           _T("Bad file descriptor")},
      {WSAEACCES,          _T("Permission denied")},
      {WSAEFAULT,          _T("Bad address")},
      {WSAEINVAL,          _T("Invalid argument")},
      {WSAEMFILE,          _T("Too many open files")},
      {WSAEWOULDBLOCK,     _T("Resource temporarily unavailable")},
      {WSAEINPROGRESS,     _T("Operation now in progress")},
      {WSAEALREADY,        _T("Operation already in progress")},
      {WSAENOTSOCK,        _T("Socket operation on non-socket")},
      {WSAEDESTADDRREQ,    _T("Destination address required")},
      {WSAEMSGSIZE,        _T("Message too long")},
      {WSAEPROTOTYPE,      _T("Protocol wrong type for socket")},
      {WSAENOPROTOOPT,     _T("Protocol not available")},
      {WSAEPROTONOSUPPORT, _T("Protocol not supported")},
      {WSAESOCKTNOSUPPORT, _T("Socket type not supported")},
      {WSAEOPNOTSUPP,      _T("Operation not supported")},
      {WSAEPFNOSUPPORT,    _T("Protocol family not supported")},
      {WSAEAFNOSUPPORT,    _T("Address family not supported by protocol family")},
      {WSAEADDRINUSE,      _T("Address already in use")},
      {WSAEADDRNOTAVAIL,   _T("Can't assign requested address")},
      {WSAENETDOWN,        _T("Network is down")},
      {WSAENETUNREACH,     _T("Network is unreachable")},
      {WSAENETRESET,       _T("Network dropped connection on reset")},
      {WSAECONNABORTED,    _T("Software caused connection abort")},
      {WSAECONNRESET,      _T("Connection reset by peer")},
      {WSAENOBUFS,         _T("No buffer space available")},
      {WSAEISCONN,         _T("Socket is already connected")},
      {WSAENOTCONN,        _T("Socket is not connected")},
      {WSAESHUTDOWN,       _T("Can't send after socket shutdown")},
      {WSAETOOMANYREFS,    _T("Too many references: can't splice")},
      {WSAETIMEDOUT,       _T("Operation timed out")},
      {WSAECONNREFUSED,    _T("Connection refused")},
      {WSAELOOP,           _T("Too many levels of symbolic links")},
      {WSAENAMETOOLONG,    _T("File name too long")},
      {WSAEHOSTDOWN,       _T("Host is down")},
      {WSAEHOSTUNREACH,    _T("No route to host")},
      {WSAENOTEMPTY,       _T("Directory not empty")},
      {WSAEPROCLIM,        _T("Too many processes")},
      {WSAEUSERS,          _T("Too many users")},
      {WSAEDQUOT,          _T("Disc quota exceeded")},
      {WSAESTALE,          _T("Stale NFS file handle")},
      {WSAEREMOTE,         _T("Too many levels of remote in path")},
      {WSASYSNOTREADY,     _T("The network subsystem is unusable")},
      {WSAVERNOTSUPPORTED, _T("The Windows Sockets DLL cannot support this app")},
      {WSANOTINITIALISED,  _T("A successful WSAStartup, has not yet been performed")},
      {WSAEDISCON,         _T("The message terminated gracefully")},
      {WSAHOST_NOT_FOUND,  _T("Authoritative Answer; Host not found")},
      {WSATRY_AGAIN,       _T("Non-Authoritative; Host not found, or SERVERFAIL")},
      {WSANO_RECOVERY,     _T("Non recoverable errors, FORMERR, REFUSED, NOTIMP")},
      {WSANO_DATA,         _T("Valid name, no data record of requested type")},
    };
  if (e < WSABASEERR || e > WSANO_DATA)
    return 0;
  for (int i = 0; i < _countof (msg); i++)
    if (e == msg[i].e)
      return msg[i].s;
  return 0;
}

int
sock::init_winsock (HINSTANCE hinst)
{
  WSADATA data;
  int e = WSAStartup (MAKEWORD (1, 1), &data);
  if (e)
    return 0;

#ifdef __XYZZY__
  WSASetBlockingHook (blocking_hook);
#endif

  if (!s_resolver.initialize (hinst)
      || !s_resolver.create (hinst))
    return 0;
  return 1;
}

void
sock::term_winsock ()
{
  WSACleanup ();
}

void
sock::initsock (SOCKET so)
{
  s_so = so;
  s_rtimeo.tv_sec = s_wtimeo.tv_sec = -1;
  s_eof_error_p = 1;
  s_rbuf.b_ptr = s_rbuf.b_base;
  s_rbuf.b_cnt = 0;
  s_wbuf.b_ptr = s_wbuf.b_base;
  s_wbuf.b_cnt = 0;
}

void
sock::closesock (int no_throw)
{
  SOCKET so = s_so;
  initsock (INVALID_SOCKET);
  if (::closesocket (so) < 0 && !no_throw)
    throw sock_error ("closesocket");
}

void
sock::close_socket (SOCKET s)
{
  ::closesocket (s);
}

sock::sock ()
{
  initsock (INVALID_SOCKET);
}

sock::sock (SOCKET so)
{
  initsock (so);
}

sock::~sock ()
{
  if (s_so != INVALID_SOCKET)
    {
      try {sflush ();} catch (sock_error &) {}
      shutdown (1, 1);
      closesock (1);
    }
}

void
sock::create (int domain, sock_type type, int proto)
{
  s_so = ::socket (domain, type, proto);
  if (s_so == INVALID_SOCKET)
    throw sock_error ("socket");
}

void
sock::close (int abort)
{
  if (s_so != INVALID_SOCKET)
    {
      if (!abort)
        {
          try
            {
              sflush ();
            }
          catch (sock_error &)
            {
              shutdown (1, 1);
              closesock (1);
              throw;
            }
        }
      shutdown (1, abort);
      closesock (abort);
    }
}

void
sock::shutdown (int how, int no_throw)
{
  if (::shutdown (s_so, how) < 0 && !no_throw)
    {
      int e = WSAGetLastError  ();
      if (e != WSAENOTCONN)
        throw sock_error ("shutdown", e);
    }
}

void
sock::cancel ()
{
  WSACancelBlockingCall ();
}

int
sock::readablep (const timeval &tv) const
{
  fd_set fds;
  FD_ZERO (&fds);
  FD_SET (s_so, &fds);

  int n = ::select (1, &fds, 0, 0, &tv);
  if (n < 0)
    {
      int e = WSAGetLastError ();
      if (!s_eof_error_p && e == WSAECONNRESET)
        return 1;
      throw sock_error ("select", e);
    }
  return n;
}

int
sock::writablep (const timeval &tv) const
{
  fd_set fds;
  FD_ZERO (&fds);
  FD_SET (s_so, &fds);

  int n = ::select (1, 0, &fds, 0, &tv);
  if (n < 0)
    throw sock_error ("select");
  return n;
}

void
sock::send (const void *buf, int len, int flags) const
{
  for (const char *b = (const char *)buf, *be = b + len; b < be;)
    {
      if (s_wtimeo.tv_sec >= 0 && !writablep (s_wtimeo))
        throw sock_error ("sock::send", WSAETIMEDOUT);
      int n = ::send (s_so, b, min (be - b, 65535), flags);
      if (n <= 0)
        throw sock_error ("send", n ? WSAGetLastError () : WSAECONNRESET);
      b += n;
    }
}

void
sock::sendto (const saddr &to, const void *buf, int len, int flags) const
{
  for (const char *b = (const char *)buf, *be = b + len; b < be;)
    {
      if (s_wtimeo.tv_sec >= 0 && !writablep (s_wtimeo))
        throw sock_error ("sock::sendto", WSAETIMEDOUT);
      int n = ::sendto (s_so, b, min (be - b, 65535), flags,
                        to.addr (), to.length ());
      if (n <= 0)
        throw sock_error ("sendto", n ? WSAGetLastError () : WSAECONNRESET);
      b += n;
    }
}

int
sock::recv (void *buf, int len, int flags) const
{
  if (s_rtimeo.tv_sec >= 0 && !readablep (s_rtimeo))
    throw sock_error ("sock::recv", WSAETIMEDOUT);
  int n = ::recv (s_so, (char *)buf, min (len, 65535), flags);
  if (n <= 0)
    {
      int e = n ? WSAGetLastError () : WSAECONNRESET;
      if (!s_eof_error_p && e == WSAECONNRESET)
        return 0;
      throw sock_error ("recv", e);
    }
  return n;
}

int
sock::recvfrom (saddr &from, void *buf, int len, int flags) const
{
  if (s_rtimeo.tv_sec >= 0 && !readablep (s_rtimeo))
    throw sock_error ("sock::recvfrom", WSAETIMEDOUT);
  int l = from.length ();
  int n = ::recvfrom (s_so, (char *)buf, min (len, 65535), flags,
                      from.addr (), &l);
  if (n <= 0)
    throw sock_error ("recvfrom", n ? WSAGetLastError () : WSAECONNRESET);
  return n;
}

void
sock::listen (int backlog) const
{
  if (::listen (s_so, backlog) < 0)
    throw sock_error ("listen");
}

SOCKET
sock::accept (saddr &addr) const
{
  int l = addr.length ();
  SOCKET so = ::accept (s_so, addr.addr (), &l);
  if (so == INVALID_SOCKET)
    throw sock_error ("accept");
  return so;
}

SOCKET
sock::accept () const
{
  SOCKET so = ::accept (s_so, 0, 0);
  if (so == INVALID_SOCKET)
    throw sock_error ("accept");
  return so;
}

void
sock::bind (const saddr &addr) const
{
  if (::bind (s_so, addr.addr (), addr.length ()) < 0)
    throw sock_error ("bind");
}

void
sock::connect (const saddr &addr) const
{
  if (::connect (s_so, addr.addr (), addr.length ()) < 0)
    throw sock_error ("connect");
}

void
sock::peeraddr (saddr &addr) const
{
  int l = addr.length ();
  if (::getpeername (s_so, addr.addr (), &l) < 0)
    throw sock_error ("getpeername");
}

void
sock::localaddr (saddr &addr) const
{
  int l = addr.length ();
  if (::getsockname (s_so, addr.addr (), &l) < 0)
    throw sock_error ("getsockname");
}

void
sock::getopt (int level, optname opt, void *val, int l) const
{
  if (::getsockopt (s_so, level, opt, (char *)val, &l) < 0)
    throw sock_error ("getsockopt");
}

void
sock::setopt (int level, optname opt, const void *val, int l) const
{
  if (::setsockopt (s_so, level, opt, (const char *)val, l) < 0)
    throw sock_error ("setsockopt");
}

void
sock::ioctl (int cmd, u_long *arg) const
{
  if (::ioctlsocket (s_so, cmd, arg) < 0)
    throw sock_error ("ioctl");
}

u_short
sock::htons (u_short x)
{
  return ::htons (x);
}

u_long
sock::htonl (u_long x)
{
  return ::htonl (x);
}

u_short
sock::ntohs (u_short x)
{
  return ::ntohs (x);
}

u_long
sock::ntohl (u_long x)
{
  return ::ntohl (x);
}

void
sock::sflush ()
{
  if (s_wbuf.b_ptr > s_wbuf.b_base)
    send (s_wbuf.b_base, s_wbuf.b_ptr - s_wbuf.b_base);
  s_wbuf.b_ptr = s_wbuf.b_base;
  s_wbuf.b_cnt = 0;
}

void
sock::sflush_buf (int c)
{
  sflush ();
  s_wbuf.b_ptr = s_wbuf.b_base + 1;
  s_wbuf.b_cnt = SOCKBUFSIZ - 1;
  *s_wbuf.b_base = c;
}

int
sock::srefill ()
{
  sflush ();
  s_rbuf.b_ptr = s_rbuf.b_base;
  s_rbuf.b_cnt = 0;
  int nread = recv (s_rbuf.b_base, SOCKBUFSIZ);
  if (!nread)
    return eof;
  s_rbuf.b_ptr = s_rbuf.b_base + 1;
  s_rbuf.b_cnt = nread - 1;
  return *s_rbuf.b_base & 0xff;
}

void
sock::sputs (const char *s)
{
  for (; *s; s++)
    sputc (*s);
}

int
sock::sgets (char *buf, size_t size)
{
  if (int (size) <= 0)
    return 0;
  char *b = buf;
  char *const be = buf + size - 1;
  while (b < be)
    {
      int c = sgetc ();
      if (c == eof)
        break;
      *b++ = c;
      if (c == '\n')
        break;
    }
  *b = 0;
  return b - buf;
}

void
sock::sungetc (int c)
{
  if (c >= 0 && s_rbuf.b_ptr > s_rbuf.b_base)
    {
      *--s_rbuf.b_ptr = (char)c;
      s_rbuf.b_cnt++;
    }
}

hostent *
sock::netdb::host (const char *hostname)
{
  return s_resolver.lookup_host (hostname);
}

hostent *
sock::netdb::host (const void *addr, int addrlen, int type)
{
  return s_resolver.lookup_host (addr, addrlen, type);
}

servent *
sock::netdb::serv (const char *service, const char *proto)
{
  return s_resolver.lookup_serv (service, proto);
}
