const String INDEX_HTML = "<html><body><form action=\"/config\" method=\"post\">  <label for=\"delay\">delay (ms):</label><br>  <input type=\"number\" id=\"delay\" name=\"delay\" value=\"100\"><br>  <input type=\"submit\" value=\"Set\"></form> <form action=\"/restart\" method=\"post\">  <label for=\"seed\">seed:</label><br>  <input type=\"number\" id=\"seed\" name=\"seed\" value=\"\"><br>  <input type=\"submit\" value=\"Restart\"></form><form action=\"/reset\" method=\"post\">  <input type=\"submit\" value=\"Reset\"></form><form action=\"/test\" method=\"post\">  <input type=\"submit\" value=\"Test\"></form> <button onclick=\"window.location.href='/update';\">OTA</button></body></html>";