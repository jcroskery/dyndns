# dyndns
Dynamic DNS using Cloudflare API.

There is a PHP version and a C version of the program.

To use the C version of this program, you must pass at least 3 command line arguments:

The first: Your Cloudflare account's email address, e.g. "X-Auth-Email: youremail@example.com"

The second: Your 32 digit Cloudflare API key, e.g. "X-Auth-Key: 00000000000000000000000000000000"

The third: Domain name you wish to update with your IP, e.g. "example.com" or "www.example.com"

Other arguments will be treated in the same way as the third argument
