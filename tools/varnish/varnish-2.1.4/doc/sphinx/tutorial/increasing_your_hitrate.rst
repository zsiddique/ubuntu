.. _tutorial-increasing_your_hitrate:

Achieving a high hitrate
------------------------

Now that Varnish is up and running, and you can access your web
application through Varnish. Unless your application is specifically
written to work behind a web accelerator you'll probably need to do
some changes to either the configuration or the application in order
to get a high hit rate in Varnish.

Varnish will not cache your data unless it's absolutely sure it is
safe to do so. So, for you to understand how Varnish decides if and
how to cache a page I'll guide you through a couple of tools that you
will find useful.

Note that you need a tool to see what HTTP headers fly between you and
the web server. If you have Varnish the easiest is to use varnishlog
and varnishtop but sometimes a client-side tool makes sense. Here are
the ones I use.

Tool: varnistop
~~~~~~~~~~~~~~~

You can use varnishtop to identify what URLs are hitting the backend
the most. ``varnishtop -i txurl`` is a essential command. You can see
some other examples of varnishtop usage in :ref:`tutorial-statistics`.


Tool: varnishlog
~~~~~~~~~~~~~~~~

When you have identified the an URL which is frequently sent to the
backend you can use varnishlog to have a look at the whole request.
``varnishlog -c -o /foo/bar`` will give the whole (-o) requests coming
from the client (-c) matching /foo/bar. 


Tool: lwp-request
~~~~~~~~~~~~~~~~~

lwp-request is part of The World-Wide Web library for Perl. It's
couple of really basic programs that can execute a HTTP request and
give you the result. I mostly use two programs, GET and HEAD.

vg.no was the first site to use Varnish and the people running Varnish
there are quite cluefull. So its interesting to look at their HTTP
Headers. Lets send a GET request for their home page.::

  $ GET -H 'Host: www.vg.no' -Used http://vg.no/
  GET http://vg.no/
  Host: www.vg.no
  User-Agent: lwp-request/5.834 libwww-perl/5.834
  
  200 OK
  Cache-Control: must-revalidate
  Refresh: 600
  Title: VG Nett - Forsiden - VG Nett
  X-Age: 463
  X-Cache: HIT
  X-Rick-Would-Never: Let you down
  X-VG-Jobb: http://www.finn.no/finn/job/fulltime/result?keyword=vg+multimedia Merk:HeaderNinja
  X-VG-Korken: http://www.youtube.com/watch?v=Fcj8CnD5188
  X-VG-WebCache: joanie
  X-VG-WebServer: leon

OK. Let me explain what it does. GET usually send off HTTP 0.9
requests, which lack the Host header. So I add a Host header with the
-H option. -U print request headers, -s prints response status -e
prints repsonse headers and -d discards the actual content. We dont
really care about the content, only the headers.

As you can see VG ads quite a bit of information in their
headers. Some of the headers, like the X-Rick-Would-Never are specific
to vg.no and their somewhat odd sense of humour. Others, like the
X-VG-Webcache are for debugging purposes. 

So, to check whether a site sets cookies for a specific URL just do::

  GET -Used http://example.com/ |grep ^Set-Cookie

Tool: Live HTTP Headers
~~~~~~~~~~~~~~~~~~~~~~~

There is also a plugin for Firefox. *Live HTTP Headers* can show you
what headers are beeing sent and recieved. Live HTTP Headers can be
found at https://addons.mozilla.org/en-US/firefox/addon/3829/ or by
googling "Live HTTP Headers".


The role of HTTP Headers
~~~~~~~~~~~~~~~~~~~~~~~~

Varnish considers itself part of the actual webserver, since its under
your control. The role of *surrogate origin cache* is not really well
defined by the IETF so RFC 2616 doesn't always tell us what we should
do.

Cache-Control
~~~~~~~~~~~~~

The Cache-Control instructs caches how to handle the content. Varnish
cares about the *max-age* parameter and uses it to calculate the TTL
for an object. 

"Cache-Control: nocache" is ignored but if you need this you can
easyli add support for it.

So make sure use issue a Cache-Control header with a max-age
header. You can have a look at what Varnish Softwares drupal server
issues:::

  $ GET -Used http://www.varnish-software.com/|grep ^Cache-Control
  Cache-Control: public, max-age=600

Age
~~~

Varnish adds a Age header to indicate how long the object has been
kept inside Varnish. You can grep out Age from varnishlog like this::

  varnishlog -i TxHeader -I ^Age

Overriding the time-to-live (ttl)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Sometimes your backend will misbehave. It might, depending on your
setup, be easier to override the ttl in Varnish then to fix your
somewhat cumbersome backend. 

You need VCL to identify the objects you want and then you set the
beresp.ttl to whatever you want.::

  sub vcl_fetch {
      if (req.url ~ "^/legacy_broken_cms/") {
          set beresp.ttl = 5d;
      }
  }

Cookies
~~~~~~~

Varnish will not cache a object comming from the backend with a
Set-Cookie header present. Also, if the client sends a Cookie header,
Varnish will bypass the cache and go directly to the backend.

This can be overly conservative. A lot of sites use Google Analytics
(GA) to analyse their traffic. GA sets a cookie to track you. This
cookie is used by the client side java script and is therefore of no
interest to the server. 

For a lot of web application it makes sense to completly disregard the
cookies unless you are accessing a special part of the web site. This
VCL snipplet in vcl_recv will disregard cookies unless you are
accessing /admin/.::

  if ( !( req.url ~ ^/admin/) ) {
    unset req.http.Cookie;
  }

Quite simple. If, however, you need to do something more complicated,
like removing one out of several cookies, things get
difficult. Unfornunatly Varnish doesn't have good tools for
manipulating the Cookies. We have to use regular expressions to do the
work. If you are familiar with regular expressions you'll understand
whats going on. If you don't I suggest you either pick up a book on
the subject, read through the *pcrepattern* man page or read through
one of many online guides.

Let me show you what Varnish Software uses. We use some cookies for
Google Analytics tracking and similar tools. The cookies are all set
and used by Javascript. Varnish and Drupal doesn't need to see those
cookies and since Varnish will cease caching of pages when the client
sends cookies we will discard these unnecessary cookies in VCL. 

In the following VCL we discard all cookies that start with a
underscore.::

  // Remove has_js and Google Analytics __* cookies.
  set req.http.Cookie = regsuball(req.http.Cookie, "(^|;\s*)(_[_a-z]+|has_js)=[^;]*", "");
  // Remove a ";" prefix, if present.
  set req.http.Cookie = regsub(req.http.Cookie, "^;\s*", "");

Let me show you an example where we remove everything the the cookies
named COOKIE1 and COOKIE2 and you can marvel at it.::

  sub vcl_recv {
    if (req.http.Cookie) {
      set req.http.Cookie = ";" req.http.Cookie;
      set req.http.Cookie = regsuball(req.http.Cookie, "; +", ";");
      set req.http.Cookie = regsuball(req.http.Cookie, ";(COOKIE1|COOKIE2)=", "; \1=");
      set req.http.Cookie = regsuball(req.http.Cookie, ";[^ ][^;]*", "");
      set req.http.Cookie = regsuball(req.http.Cookie, "^[; ]+|[; ]+$", "");

      if (req.http.Cookie == "") {
          remove req.http.Cookie;
      }
  }

The example is taken from the Varnish Wiki, where you can find other
scary examples of what can be done i VCL.

Vary
~~~~

The Vary header is sent by the web server to indicate what makes a
HTTP object Vary. This makes a lot of sense with headers like
Accept-Encoding. When a server issues a "Vary: Accept-Encoding" it
tells Varnish that its needs to cache a separate version for every
different Accept-Encoding that is coming from the clients. So, if a
clients only accepts gzip encoding Varnish won't serve the version of
the page encoded with the deflate encoding.

The problem is that the Accept-Encoding field contains a lot of
different encodings. If one browser sends::

  Accept-Encodign: gzip,deflate

And another one sends::

  Accept-Encoding:: deflate,gzip

Varnish will keep two variants of the page requested due to the
different Accept-Encoding headers. Normalizing the accept-encoding
header will sure that you have as few variants as possible. The
following VCL code will normalize the Accept-Encoding headers.::

    if (req.http.Accept-Encoding) {
        if (req.url ~ "\.(jpg|png|gif|gz|tgz|bz2|tbz|mp3|ogg)$") {
            # No point in compressing these
            remove req.http.Accept-Encoding;
        } elsif (req.http.Accept-Encoding ~ "gzip") {
            set req.http.Accept-Encoding = "gzip";
        } elsif (req.http.Accept-Encoding ~ "deflate") {
            set req.http.Accept-Encoding = "deflate";
        } else {
            # unkown algorithm
            remove req.http.Accept-Encoding;
        }
    }

The code sets the Accept-Encoding header from the client to either
gzip, deflate with a preference for gzip.

Pitfall - Vary: User-Agent
~~~~~~~~~~~~~~~~~~~~~~~~~~

Some applications or application servers send *Vary: User-Agent* along
with their content. This instructs Varnish to cache a separate copy
for every variation of User-Agent there is. There are plenty. Even a
single patchlevel of the same browser will generate at least 10
different User-Agent headers based just on what operating system they
are running. 

So if you *really* need to Vary based on User-Agent be sure to
normalize the header or your hit rate will suffer badly. Use the above
code as a template.

.. _tutorial-increasing_your_hitrate-pragma:

Pragma
~~~~~~


HTTP 1.0 server might send "Pragma: nocache". Varnish ignores this
header. You could easly add support for this header in VCL.

In vcl_fetch::

  if (beresp.http.Pragma ~ "nocache") {
     pass;
  }

Authorization
~~~~~~~~~~~~~

If Varnish sees a Authorization header it will pass the request. If
this is not what you want you can unset the header.


Normalizing your namespace
~~~~~~~~~~~~~~~~~~~~~~~~~~

Some sites are accessed via lots of
hostnames. http://www.varnish-software.com ,
http://varnish-software.com and http://varnishsoftware.com/ all point
at the same site. Since Varnish doesn't know they are different
Varnish will cache different versions of every page for every
hostname. You can mitigate this in your web server configuration by setting
up redirects or by using the following VCL:::

  if (req.http.host ~ "^(www.)?varnish-?software.com") {
    set req.http.host = "varnish-software.com";
  }

.. _tutorial-increasing_your_hitrate-purging:

Purging
~~~~~~~

One of the most effective way of increasing your hit ratio is to
increase the time-to-live (ttl) of your objects. In this twitterific
day of age serving content that is outdated is bad for business.

The solution is to notify Varnish when there is fresh content
available. This can be done through two mechanisms. HTTP purging and
bans. First, let me explain the HTTP purges. 


HTTP Purges
~~~~~~~~~~~

An HTTP purge is similar to a HTTP GET request, except that the
*method* is PURGE. Actually you can call the method whatever you'd
like, but most people refer to this as purging. Squid supports the
same mechanism. In order to support purging in Varnish you need the
following VCL in place:::

  acl purge {
	  "localhost";
	  "192.168.55.0/24";
  }
  
  sub vcl_recv {
      	  # allow PURGE from localhost and 192.168.55...

	  if (req.request == "PURGE") {
		  if (!client.ip ~ purge) {
			  error 405 "Not allowed.";
		  }
		  return (lookup);
	  }
  }
  
  sub vcl_hit {
	  if (req.request == "PURGE") {
	          # Note that setting ttl to 0 is magical.
                  # the object is zapped from cache.
		  set obj.ttl = 0s;
		  error 200 "Purged.";
	  }
  }
  
  sub vcl_miss {
	  if (req.request == "PURGE") {

		  error 404 "Not in cache.";
	  }
  }

As you can see we have used to new VCL subroutines, vcl_hit and
vcl_miss. When we call lookup Varnish will try to lookup the object in
its cache. It will either hit an object or miss it and so the
corresponding subroutine is called. In vcl_hit the object that is
stored in cache is available and we can set the TTL.

So for vg.no to invalidate their front page they would call out to
varnish like this:::

  PURGE / HTTP/1.0
  Host: vg.no

And Varnish would then discard the front page. If there are several
variants of the same URL in the cache however, only the matching
variant will be purged. To purge a gzip variant of the same page the
request would have to look like this:::

  PURGE / HTTP/1.0
  Host: vg.no
  Accept-Encoding: gzip

Bans
~~~~

There is another way to invalidate content. Bans. You can think of
bans as a sort of a filter. You *ban* certain content from being
served from your cache. You can ban content based on any metadata we
have.

Support for bans is built into Varnish and available in the CLI
interface. For VG to ban every png object belonging on vg.no they could
issue:::

  purge req.http.host == "vg.no" && req.http.url ~ "\.png$"

Quite powerful, really.

Bans are checked when we hit an object in the cache, but before we
deliver it. An object is only checked against newer bans. If you have
a lot of objects with long TTL in your cache you should be aware of a
potential performance impact of having many bans.

You can also add bans to Varnish via HTTP. Doing so requires a bit of VCL.::

  sub vcl_recv {
	  if (req.request == "BAN") {
                  # Same ACL check as above:
		  if (!client.ip ~ purge) {
			  error 405 "Not allowed.";
		  }
		  purge("req.http.host == " req.http.host 
		        "&& req.url == " req.url);

		  # Throw a synthetic page so the
                  # request wont go to the backend.
		  error 200 "Ban added"
	  }
  }

This VCL sniplet enables Varnish to handle a HTTP BAN method. Adding a
ban on the URL, including the host part.
