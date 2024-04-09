/**
 * Webserver for the FPGC
 * Uses multiple sockets to handle multiple connections (except for socket 7 which is reserved by netHID)
 * Note that all files are stored in the filesystem per word and not in bytes
 *  So currently the only the rightmost byte of each file are sent over the network
 *  This is to stay compatible with text files
*/

#define word char

#include "lib/math.c"
#include "lib/stdlib.c"
#include "lib/sys.c"
#include "lib/brfs.c"
#include "lib/wiz5500.c"

#define WIZNET_IP 213

char wiz_rbuf[WIZNET_MAX_RBUF];

word percentage_done(word remaining, word full)
{
  word x = remaining * 100;
  return 100 - MATH_divU(x, full);
}

/**
 * Write a file from the filesystem to a socket
 * Sends the file in parts of WIZNET_MAX_TBUF
 * Assumes the filepath is valid
*/
void write_file_from_fs(word s, char* path, word filesize)
{
  // Open the file
  word fd = fs_open(path);
  if (fd == -1)
  {
    // Should not happen
    bdos_println("UNEXPECTED: File not found!");
    return;
  }

  word fullsize = filesize;

  // Read file in chunks of WIZNET_MAX_TBUF
  word file_buffer[WIZNET_MAX_RBUF];
  word chunk_to_read;

  char dbuf[10]; // Percentage done for progress indication
  dbuf[0] = 0;

  while (filesize > 0)
  {
    chunk_to_read = filesize > WIZNET_MAX_RBUF ? WIZNET_MAX_RBUF : filesize;
    fs_read(fd, (char*)file_buffer, chunk_to_read);
    wiz_write_data(s, file_buffer, chunk_to_read);
    filesize -= chunk_to_read;

    // Calculate percentage done
    word percentage = percentage_done(filesize, fullsize);

    // Remove previous percentage
    word i = strlen(dbuf);
    while (i > 0)
    {
      bdos_printc(0x8); // Backspace
      i--;
    }
    if (strlen(dbuf) != 0)
    {
      bdos_printc(0x8); // Backspace
    }
    itoa(percentage, dbuf);
    bdos_print(dbuf);
    bdos_printc('%');
  }

  bdos_printc(' '); // Add space after percentage

  // Close file
  fs_close(fd);
}


/**
 * Send a 404 response
 * Could be extended by sending a custom 404 page if found in the filesystem
*/
void send_404_response(word s)
{
  char* response_header = "HTTP/1.1 404 Not Found\nServer: FPGC/2.0\nContent-Type: text/html\n\n";
  wiz_write_data(s, response_header, strlen(response_header));
  char* response_body = "<!DOCTYPE html><html><head><title>ERROR404</title></head><body>ERROR 404: Could not find the requested file or directory :(</body></html>";
  wiz_write_data(s, response_body, strlen(response_body));
  // Disconnect after sending a response
  wiz_send_cmd(s, WIZNET_CR_DISCON);
}


/**
 * Parse the file path from a request header in rbuf
 * Writes field to pbuf
 * File path is assumed to be the second word in the request header
*/
void parse_file_path(char* rbuf, char* pbuf)
{
  strtok(rbuf, " "); // Skip the first word
  char* file_path = strtok((char*)-1, " ");

  if ((word)file_path != -1 && strlen(file_path) < MAX_PATH_LENGTH)
  {
    strcpy(pbuf, file_path);
  }
  else
  {
    strcpy(pbuf, "/");
  }
}

/**
 * Send directory listing to socket s as part of send_dir
 * Only sends the body of the response and should not disconnect the socket
 * Assumes path is a valid directory
*/
void send_list_dir(word s, char* path)
{
  // Write start of html page
  char* html_start = "<!DOCTYPE html><html><body><h2>";
  wiz_write_data(s, html_start, strlen(html_start));

  // Print path as header
  wiz_write_data(s, path, strlen(path));

  // Start table
  char* table_start = "</h2><table><tr><th>Name</th><th>Size</th></tr>";
  wiz_write_data(s, table_start, strlen(table_start));

  // Obtain directory entries
  struct brfs_dir_entry entries[MAX_DIR_ENTRIES];
  word num_entries = fs_readdir(path, (char*)entries);

  // Keep track of the longest filename for formatting
  word max_filename_length = 0;

  // Sort entries by filename
  word i, j;
  for (i = 0; i < num_entries - 1; i++)
  {
    for (j = 0; j < num_entries - i - 1; j++)
    {
      char decompressed_filename1[17];
      strdecompress(decompressed_filename1, entries[j].filename);

      char decompressed_filename2[17];
      strdecompress(decompressed_filename2, entries[j + 1].filename);

      // Update max_filename_length
      // This works because . is always the first entry (skipped by this check)
      if (strlen(decompressed_filename2) > max_filename_length)
      {
        max_filename_length = strlen(decompressed_filename2);
      }

      // Sort by filename
      if (strcmp(decompressed_filename1, decompressed_filename2) > 0)
      {
        // Swap filenames
        struct brfs_dir_entry temp = entries[j];
        entries[j] = entries[j + 1];
        entries[j + 1] = temp;
      }
    }
  }

  // Loop through sorted entries and print them
  for (i = 0; i < num_entries; i++)
  {
    // Send start of table row
    char* table_row_start = "<tr><td style=\"padding:0 15px 0 15px;\"><a href=\"";
    wiz_write_data(s, table_row_start, strlen(table_row_start));

    // Create entry string
    char entry_str[128];

    // Add filename
    struct brfs_dir_entry entry = entries[i];
    char filename[17];
    strdecompress(filename, entry.filename);
    strcpy(entry_str, filename);

    // Add / if directory
    if ((entry.flags & 0x01) == 1)
    {
      strcat(entry_str, "/");
    }

    // Add end of HTML link tag
    strcat(entry_str, "\">");

    // Add filename again for the link text
    strcat(entry_str, filename);
    if ((entry.flags & 0x01) == 1)
    {
      strcat(entry_str, "/");
    }

    // End the hyperlink
    strcat(entry_str, "</a></td><td style=\"padding:0 15px 0 15px;\">");

    // Add filesize if file
    if ((entry.flags & 0x01) == 0)
    {
      char filesize_str[11];
      itoa(entry.filesize, filesize_str);
      strcat(entry_str, filesize_str);
    }

    // End the table row
    strcat(entry_str, "</td></tr>\n");
    
    // Write the entry to the socket
    wiz_write_data(s, entry_str, strlen(entry_str));
  }

  // write end of html page
  wiz_write_data(s, "</table></body></html>", 22);
}

/**
 * Serve a file
 * Sends the response to socket s
 * Includes the content length in the header so the client knows how large a download is
 * Assumes the file is valid
*/
void serve_file(word s, char* path)
{
  // Get file size
  struct brfs_dir_entry* entry = (struct brfs_dir_entry*)fs_stat(path);
  if ((word)entry == -1)
  {
    // Should not happen, so just disconnect the session
    bdos_println("UNEXPECTED: File not found!");
    wiz_send_cmd(s, WIZNET_CR_DISCON);
  }
  word filesize = entry->filesize;

  bdos_print("200 ");

  // Write header (currently omitting content type)
  char header[128];
  strcpy(header, "HTTP/1.1 200 OK\nServer: FPGC/2.0\nContent-Length: ");
  
  char filesize_str[12];
  itoa(filesize, filesize_str);
  strcat(header, filesize_str);
  strcat(header, "\n\n");

  wiz_write_data(s, header, strlen(header));

  // Write the response from filesystem
  write_file_from_fs(s, path, filesize);
  bdos_println("Done");

  // Disconnect after sending a response
  wiz_send_cmd(s, WIZNET_CR_DISCON);
}

/**
 * Serve a directory
 * Sends the response to socket s
 * Assumes the directory is valid
*/
void serve_dir(word s, char* path)
{
  // If path does not end with a slash, redirect to the same path with a slash
  if (path[strlen(path) - 1] != '/')
  {
    strcat(path, "/");
    char* response = "HTTP/1.1 301 Moved Permanently\nLocation: ";
    bdos_print("Redirect to ");
    bdos_println(path);
    wiz_write_data(s, response, strlen(response));
    wiz_write_data(s, path, strlen(path));
    wiz_write_data(s, "\n", 1);
  }
  else
  {
    bdos_print("200 ");
    // Write header (currently omitting content type)
    char* header = "HTTP/1.1 200 OK\nServer: FPGC/2.0\n\n";
    wiz_write_data(s, header, strlen(header));
    send_list_dir(s, path);
    bdos_println("Done");
  }

  // Disconnect after sending a response
  wiz_send_cmd(s, WIZNET_CR_DISCON);
}

/**
 * Serve a path from the file system
 * Sends the response to socket s
*/
void serve_path(word s, char* path)
{
  // Redirect "/" to "/index.html"
  if (strcmp(path, "/") == 0)
  {
    // Send an actual redirect to the browser
    char *response = "HTTP/1.1 301 Moved Permanently\nLocation: /index.html\n";
    bdos_println("Redirect to /index.html\n");
    wiz_write_data(s, response, strlen(response));

    // Disconnect after sending the redirect
    wiz_send_cmd(s, WIZNET_CR_DISCON);
    return;
  }

  // Check if the path is a directory
  char path_copy[MAX_PATH_LENGTH];
  strcpy(path_copy, path); // Make a copy as stat remove trailing slashes
  struct brfs_dir_entry* entry = (struct brfs_dir_entry*)fs_stat(path_copy);
  if ((word)entry == -1)
  {
    bdos_println("Path not found");
    send_404_response(s);
    return;
  }

  if ((entry->flags & 0x01) == 0)
  {
    // File
    serve_file(s, path);
  }
  else
  {
    // Directory
    serve_dir(s, path);
  }
} 

/**
 * Handle a session on socket s
*/
void handle_session(word s)
{
  // Size of received data
  word rsize = wiz_get_sock_reg_16(s, WIZNET_SnRX_RSR);

  // Disconnect on no data
  if (rsize == 0)
  {
    wiz_send_cmd(s, WIZNET_CR_DISCON);
    return;
  }

  // Read data into buffer
  wiz_read_recv_data(s, wiz_rbuf, rsize);

  // Read rbuf for requested page
  char path[MAX_PATH_LENGTH]; // Buffer for path name
  parse_file_path(wiz_rbuf, path);

  bdos_print(path);
  bdos_print(" ");

  serve_path(s, path);
}

/**
 * Reinitialize the W5500
*/
word reinit_w5500()
{
  word ip_addr[4] = {192, 168, 0, WIZNET_IP};
  word gateway_addr[4] = {192, 168, 0, 1};
  word mac_addr[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0x24, 0x64};
  word sub_mask[4] = {255, 255, 255, 0};

  wiz_init(ip_addr, gateway_addr, mac_addr, sub_mask);
}

/**
 * Open all sockets in TCP Server mode at port 80
*/
word open_all_sockets_tcp_server()
{
  word s;
  for (s = 0; s < 7; s++)
  {
    wiz_init_socket_tcp_host(s, 80);
  }
}

/**
 * Main function
*/
int main() 
{
  bdos_println("FPGC Webserver v2.0");

  reinit_w5500();
  open_all_sockets_tcp_server();

  // Main loop
  while (1)
  {
    if (hid_checkfifo())
    {
      // Return on any key press
      hid_fiforead();
      return 'q';
    }

    // Handle sockets 0-6
    word s_status;
    word s;
    for (s = 0; s < 7; s++)
    {
      s_status = wiz_get_sock_reg_8(s, WIZNET_SnSR);

      if (s_status == WIZNET_SOCK_CLOSED)
      {
        // Open the socket when closed
        // Set socket s in TCP Server mode at port 80
        wiz_init_socket_tcp_host(s, 80);
      }
      else if (s_status == WIZNET_SOCK_ESTABLISHED)
      {
        // Handle session when a connection is established
        handle_session(s);
        // Afterwards, reinitialize socket
        wiz_init_socket_tcp_host(s, 80);
      }
      else if (s_status == WIZNET_SOCK_LISTEN || s_status == WIZNET_SOCK_SYNSENT || s_status == WIZNET_SOCK_SYNRECV)
      {
        // Do nothing in these cases
      }
      else
      {
        // In other cases, reset the socket
        // Set socket s in TCP Server mode at port 80
        wiz_init_socket_tcp_host(s, 80);
      }
    }
    delay(20);
  }

  return 'q';
}

void interrupt()
{
  // Handle all interrupts
  word i = get_int_id();
  switch(i)
  {
    case INTID_TIMER1:
      timer1Value = 1;  // Notify ending of timer1
      break;
  }
}