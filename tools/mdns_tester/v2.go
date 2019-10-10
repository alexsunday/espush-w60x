package main

import (
	"context"
	"fmt"
	"github.com/grandcat/zeroconf"
	"time"
)

func mDNSBrowser() {
	fmt.Println("wait mDNS record.")
	// Discover all services on the network (e.g. _workstation._tcp)
	resolver, err := zeroconf.NewResolver(nil)
	if err != nil {
		fmt.Println("Failed to initialize resolver:", err.Error())
		return
	}

	ch := make(chan *zeroconf.ServiceEntry)
	go func(results <-chan *zeroconf.ServiceEntry) {
		for entry := range results {
			fmt.Println("recv record.", entry)
			fmt.Printf("Service: %s,%s,%s\n",
				entry.ServiceName(),
				entry.ServiceInstanceName(),
				entry.ServiceTypeName(),
			)
			fmt.Printf("Record: %s, %s, %s\n",
				entry.Instance,
				entry.Service,
				entry.Domain,
			)
			fmt.Printf("HostInfo: %s, %d, %v\n",
				entry.HostName,
				entry.Port,
				entry.Text,
			)
			fmt.Printf("Addr: %s\n", entry.AddrIPv4[0].String())
		}
	}(ch)

	ctx, cancel := context.WithTimeout(context.Background(), time.Second*1500)
	defer cancel()
	//err = resolver.Browse(ctx, "light._http", "local.", ch)
	err = resolver.Browse(ctx, "light.espush._tcp", "", ch)
	if err != nil {
		fmt.Println("Failed to browse:", err.Error())
		return
	}

	<-ctx.Done()
}


func mDNSLookup() {
	fmt.Println("wait mDNS record.")
	// Discover all services on the network (e.g. _workstation._tcp)
	resolver, err := zeroconf.NewResolver(nil)
	if err != nil {
		fmt.Println("Failed to initialize resolver:", err.Error())
		return
	}

	ch := make(chan *zeroconf.ServiceEntry)
	go func(results <-chan *zeroconf.ServiceEntry) {
		for entry := range results {
			fmt.Println("recv record.", entry)
			fmt.Printf("Service: %s,%s,%s\n",
				entry.ServiceName(),
				entry.ServiceInstanceName(),
				entry.ServiceTypeName(),
			)
			fmt.Printf("Record: %s, %s, %s\n",
				entry.Instance,
				entry.Service,
				entry.Domain,
			)
			fmt.Printf("HostInfo: %s, %d, %v\n",
				entry.HostName,
				entry.Port,
				entry.Text,
			)
			fmt.Printf("Addr: %s\n", entry.AddrIPv4[0].String())
		}
	}(ch)

	ctx, cancel := context.WithTimeout(context.Background(), time.Second*1500)
	defer cancel()
	err = resolver.Lookup(ctx, "", "light.espush._tcp", "", ch)
	if err != nil {
		fmt.Println("Failed to browse:", err.Error())
		return
	}

	<-ctx.Done()
}

func main() {
	mDNSLookup()
	//mDNSBrowser()
}
