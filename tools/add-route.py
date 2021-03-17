#!/usr/bin/python3

import pyads
import argparse

parser = argparse.ArgumentParser(description='Add route to remote TwinCAT PLC')
required = parser.add_argument_group('required arguments')
required.add_argument('--route_name', required=True, type=str)
required.add_argument('--sender_ams', required=True, type=str)
required.add_argument('--route_dest', required=True, type=str)
required.add_argument('--plc_username', required=True,  type=str)
required.add_argument('--plc_password', required=True, type=str)
required.add_argument('--plc_ip', required=True, type=str)

args = parser.parse_args()

pyads.add_route_to_plc(sending_net_id=args.sender_ams, adding_host_name=args.route_dest,ip_address=args.plc_ip, username=args.plc_username, password=args.plc_password, route_name=args.route_name)
