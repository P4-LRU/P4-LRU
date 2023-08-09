import time
import sys

threshold = 3

################################################################################

table = bfrt.lru_mon.pipe1.Ingress1.counter_threshold
table.clear()
table.mod(0, threshold * 100)

table = bfrt.lru_mon.pipe1.Ingress1.simple_send_table
table.clear()
table.add_with_simple_send(128, 144)
table.add_with_simple_send(144, 128)

table = bfrt.lru_mon.pipe1.Ingress1.simple_send_table_to_re
table.clear()
table.add_with_simple_send(128, 44)
table.add_with_simple_send(144, 36)

table = bfrt.lru_mon.pipe1.Ingress1.filter_counter_buckets_00
table.clear()
table = bfrt.lru_mon.pipe1.Ingress1.filter_counter_buckets_01
table.clear()
table = bfrt.lru_mon.pipe1.Ingress1.filter_counter_buckets_10
table.clear()
table = bfrt.lru_mon.pipe1.Ingress1.filter_counter_buckets_11
table.clear()

table = bfrt.lru_mon.pipe1.Ingress1.filter_timestamp_buckets_00
table.clear()
table = bfrt.lru_mon.pipe1.Ingress1.filter_timestamp_buckets_01
table.clear()
table = bfrt.lru_mon.pipe1.Ingress1.filter_timestamp_buckets_10
table.clear()
table = bfrt.lru_mon.pipe1.Ingress1.filter_timestamp_buckets_11
table.clear()

################################################################################

table = bfrt.lru_mon.pipe2.Ingress2.simple_send_table_from_re
table.clear()
table.add_with_simple_send_from_re(44, 144)
table.add_with_simple_send_from_re(36, 128)

table = bfrt.lru_mon.pipe2.Egress2.fp_1
table.clear()

table = bfrt.lru_mon.pipe2.Egress2.fp_2
table.clear()

table = bfrt.lru_mon.pipe2.Egress2.fp_3
table.clear()

table = bfrt.lru_mon.pipe2.Egress2.reg_dfa
table.clear()

table = bfrt.lru_mon.pipe2.Egress2.counter_1
table.clear()

table = bfrt.lru_mon.pipe2.Egress2.counter_2
table.clear()

table = bfrt.lru_mon.pipe2.Egress2.counter_3
table.clear()
