package main

import (
	"fmt"
	"os"

	"github.com/hashicorp/mdns"
)

func _main() {
	var err error

	host, _ := os.Hostname()
	info := []string{"My awesome service"}
	_, err = mdns.NewMDNSService(host, "_foobar._tcp", "", "", 8000, nil, info)
	if err != nil {
		fmt.Println("new dns service failed.")
		return
	}
	fmt.Println("New service success.")

	ch := make(chan *mdns.ServiceEntry, 4)
	go func() {
		for entry := range ch {
			fmt.Printf("got new entry: %v\n", entry)
		}
	}()

	err = mdns.Lookup("_foobar._tcp", ch)
	if err != nil {
		fmt.Println("mdns lookup failed.")
		return
	}

	close(ch)
}
