package main

import (
	"context"
	"flag"
	"fmt"
	"github.com/gin-gonic/gin"
	"github.com/grandcat/zeroconf"
	"net/http"
	"time"
)

var (
	mode = flag.String("mode", "discovery", "run mode, discovery/register")
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
			fmt.Printf("Service: %s, %s, %s\n",
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
			fmt.Printf("Addr: %s\n\n", entry.AddrIPv4[0].String())
		}
	}(ch)

	ctx, cancel := context.WithTimeout(context.Background(), time.Second*1500)
	defer cancel()
	//err = resolver.Browse(ctx, "light._http", "local.", ch)
	err = resolver.Browse(ctx, "", "", ch)
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
			fmt.Printf("Service: %s, %s, %s\n",
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
			fmt.Printf("Addr: %s\n\n", entry.AddrIPv4[0].String())
		}
	}(ch)

	ctx, cancel := context.WithTimeout(context.Background(), time.Second*1500)
	defer cancel()
	err = resolver.Lookup(ctx, "", "_espush._tcp", "", ch)
	if err != nil {
		fmt.Println("Failed to browse:", err.Error())
		return
	}

	<-ctx.Done()
}

func SvcRegister() {
	fmt.Println("service begin.")
	var webPort = 12301
	server, err := zeroconf.Register("lighter",
		"_espush._tcp",
		"local.",
		webPort,
		[]string{"txtv=0", "lo=1", "la=2"},
		nil)
	if err != nil {
		panic(err)
	}
	defer server.Shutdown()

	app := gin.Default()
	app.GET("/device/info", func(c *gin.Context) {
		c.JSON(http.StatusOK, gin.H{
			"device_type": "light",
			"chip_id": "W60X40D63C1AFDC2",
		})
	})

	err = app.Run(fmt.Sprintf("0.0.0.0:%d", webPort))
	if err != nil {
		panic(err)
	}
}

func main() {
	flag.Parse()

	if mode == nil {
		fmt.Println("run mode empty.")
		return
	}

	var runMode = *mode
	if runMode == "lookup" {
		mDNSLookup()
	} else if runMode == "browser" {
		mDNSBrowser()
	} else if runMode == "register" {
		SvcRegister()
	}
}
