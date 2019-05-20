# dyndns
Dynamic DNS using Cloudflare API.

There is a PHP version and a C version of the program.

To use the C version of this program, you must pass at least 4 command line arguments:

The first: Whether you want to use the caching feature of this program to cache your Cloudflare zone id and record id(s).
A value of 0 means "do not cache" and a value of 1 means "cache". 
If you are using caching and the cache is corrupted or the domain argument(s) passed to the program change, then the program will delete the cache and regenerate it.

The second: Your Cloudflare account's email address, e.g. "X-Auth-Email: youremail@example.com".

The third: Your 32 digit Cloudflare API key, e.g. "X-Auth-Key: 00000000000000000000000000000000".

The fourth: Domain name you wish to update with your IP, e.g. "example.com" or "www.example.com".

Other arguments will be treated in the same way as the fourth argument.

