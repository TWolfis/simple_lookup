/*
 * file: simple_lookup.c
 *
 * simple_lookup is a C program to experiment with DNS lookups
 *
 * License: BSD-3-Clause
 * Author: Thomas Wolfis
 */

#define MAX_HOSTNAME_LEN 253

#include <arpa/nameser.h>
#include <netinet/in.h>
#include <regex.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

struct rr_response {
  int type;
  int class;
  u_long ttl;
  int rdlength;
  const u_char *rdata;

  struct rr_response *next;
};

struct rr_response *rr_response_new() {
  struct rr_response *response =
      (struct rr_response *)malloc(sizeof(struct rr_response));
  if (response == NULL) {
    perror("Could not allocate rr_response");
  }
  return response;
};

void rr_response_free(struct rr_response *response) {
  if (response == NULL) {
    return;
  }
  free(response);
};

void rr_response_add(struct rr_response *response, struct rr_response *new) {
  if (response == NULL) {
    perror("response is NULL");
  }

  struct rr_response *current = response;
  while (current->next != NULL) {
    current = current->next;
  }
  current->next = new;
};

void rr_pretty_print_all(struct rr_response *response) {
  if (response == NULL) {
    perror("response is NULL");
  }

  struct rr_response *current = response;

  // skip the first element
  current = current->next;
  while (current != NULL) {
    printf("Type: %d ", current->type);
    printf("Class: %d ", current->class);
    printf("TTL: %lu ", current->ttl);
    printf("RDLength: %d ", current->rdlength);

    // make sure rdata is not NULL
    if (current->rdata == NULL) {
      perror("rdata is NULL");
    }

    switch (current->type) {
    case ns_t_a: // A record (IPv4 address)
      if (current->rdlength == 4) {
        printf("RData: %d.%d.%d.%d\n", current->rdata[0], current->rdata[1],
               current->rdata[2], current->rdata[3]);
      } else {
        printf("Invalid A record length\n");
      }
      break;
    case ns_t_cname: // CNAME record
      printf("RData (CNAME): %s\n", (char *)current->rdata);
      break;
    default:
      printf("RData (unknown type): ");
      for (int j = 0; j < current->rdlength; j++) {
        printf("%02x ", current->rdata[j]);
      }
      printf("\n");
    }

    current = current->next;
  }
}

void rr_response_parse(struct rr_response *response, ns_msg *msg, int ancount) {

  if (response == NULL) {
    perror("response is NULL");
  }

  // Parse the response and append it to the linked list
  for (int i = 0; i < ancount; i++) {
    ns_rr rr;
    int r = ns_parserr(msg, ns_s_an, i, &rr);

    if (r < 0) {
      perror("Could not parse response");
    }

    // get the type of the answer
    int type = ns_rr_type(rr);

    // get the class of the answer
    int class = ns_rr_class(rr);

    // get the TTL of the answer
    u_long ttl = ns_rr_ttl(rr);

    // get the length of the answer
    int rdlength = ns_rr_rdlen(rr);

    // get the rdata of the answer
    const u_char *rdata = ns_rr_rdata(rr);

    // create a new response
    struct rr_response *new = rr_response_new();
    new->type = type;
    new->class = class;
    new->ttl = ttl;
    new->rdlength = rdlength;
    new->rdata = rdata;

    // add the new response to the linked list
    rr_response_add(response, new);
  }
}

// isvalidhostname checks if a hostname is valid
int isvalidhostname(char *hostname) {
  regex_t regex;
  int reti;

  // Pattern to match a valid hostname
  const char *pattern =
      "^([a-zA-Z0-9]([a-zA-Z0-9\\-]*[a-zA-Z0-9])?\\.)+[a-zA-Z]{2,}$";

  // Compile the regex
  reti = regcomp(&regex, pattern, REG_EXTENDED);
  if (reti) {
    perror("Could not compile regex");
  }

  // Execute the regular expression
  reti = regexec(&regex, hostname, 0, NULL, 0);
  if (reti == REG_NOMATCH) {
    perror("Hostname is invalid");
    regfree(&regex);
    return 1;
  } else if (reti) {
    perror("Could not execute regex");
    regfree(&regex);
    return 1;
  }

  // Free the compiled regex
  regfree(&regex);
  return 0; // Hostname is valid
}

int main(int argc, char *argv[]) {

  char hostname[MAX_HOSTNAME_LEN + 1];

  if (argc < 2) {
    printf("Usage: %s <hostname>\n", argv[0]);
    return 1;
  }

  // make sure the given input is > 253 characters
  if (strlen(argv[1]) > 253) {
    perror("Hostname is too long");
  }

  // copy the input string into hostname
  strncpy(hostname, argv[1], MAX_HOSTNAME_LEN);
  hostname[MAX_HOSTNAME_LEN] = '\0';

  if (isvalidhostname(argv[1])) {
    return 1;
  }

  // create buffer to hold the resolver answer
  u_char *ns_buffer;
  ns_buffer = (u_char *)malloc(NS_PACKETSZ * sizeof(u_char));

  if (ns_buffer == NULL) {
    fprintf(stderr, "Could not allocate ns_buffer\n");
    return 1;
  }

  // make the DNS query
  printf("Resolving hostname %s\n", hostname);
  int response = res_query(hostname, ns_c_in, ns_t_a, ns_buffer, NS_PACKETSZ);
  if (response < 0) {
    fprintf(stderr, "Could not resolve hostname\n");
    free(ns_buffer);
    return 1;
  }

  // parse the response
  ns_msg msg;

  char dispbuf[4096];

  int r = ns_initparse(ns_buffer, response, &msg);

  if (r < 0) {
    fprintf(stderr, "Could not parse response\n");
    free(ns_buffer);
    return 1;
  }

  // get the number of answers
  int ancount = ns_msg_count(msg, ns_s_an);
  if (ancount == 0) {
    perror("No answers found");
  }

  struct rr_response *response_list = rr_response_new();
  rr_response_parse(response_list, &msg, ancount);
  printf("Response:\n");
  rr_pretty_print_all(response_list);
  rr_response_free(response_list);

  free(ns_buffer);
  return 0;
}
