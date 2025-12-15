import time

# constants
MIN_F = -10
MAX_F = 30
IDLE = "IDLE"
UP = "UP"
DOWN = "DOWN"
EMERGENCY = "EMERGENCY"

class Node:
    def __init__(self, f):
        self.floor = f
        self.next = None

class Stack:
    def __init__(self):
        self.l = [] 

    def push(self, item):
        self.l.append(item)
        # keep last 10
        if len(self.l) > 10:
            self.l.pop(0)

    def show(self):
        return self.l[::-1]

class Elevator:
    def __init__(self, n):
        self.name = n
        self.cf = 0 # current floor
        self.d = IDLE 
        self.state = "ACTIVE" 
        self.tf = 0 # target floor
        
        # queues
        self.up_h = None
        self.down_h = None
        self.v_up = None # vip up
        self.v_down = None # vip down
        
        self.hist = Stack()

    # calc furthest point (tf)
    def update_tf(self):
        last = self.cf # default
        
        if self.d == UP:
            # check vip
            if self.v_up:
                curr = self.v_up
                while curr.next: curr = curr.next
                last = curr.floor
            # check normal
            if self.up_h:
                curr = self.up_h
                while curr.next: curr = curr.next
                if curr.floor > last: last = curr.floor
                
        elif self.d == DOWN:
            if self.v_down:
                curr = self.v_down
                while curr.next: curr = curr.next
                last = curr.floor
            if self.down_h:
                curr = self.down_h
                while curr.next: curr = curr.next
                if curr.floor < last: last = curr.floor # lower is further
        
        self.tf = last

    # helper insert
    def _insert(self, head, flr, asc):
        # check dup head
        if head and head.floor == flr: return head
        
        tmp = Node(flr)
        if head is None: return tmp
        
        # check head
        if asc: check = (flr < head.floor)
        else:   check = (flr > head.floor)
        
        if check:
            tmp.next = head
            return tmp
            
        curr = head
        while curr.next:
            if curr.next.floor == flr: return head # dup
            
            if asc:
                if curr.next.floor > flr: break
            else:
                if curr.next.floor < flr: break
            curr = curr.next
            
        tmp.next = curr.next
        curr.next = tmp
        return head

    def add_req(self, flr, vip=False):
        if self.state == EMERGENCY:
            print(f"[{self.name}] EMERGENCY! ign.")
            return

        # check if we are there
        if flr == self.cf:
            print(f"[{self.name}] Doors Open at {flr}")
            self.hist.push(flr)
            return 

        # add to queues
        if flr > self.cf:
            if vip: self.v_up = self._insert(self.v_up, flr, True)
            else:   self.up_h = self._insert(self.up_h, flr, True)
            if self.d == IDLE: self.d = UP
            
        elif flr < self.cf:
            if vip: self.v_down = self._insert(self.v_down, flr, False)
            else:   self.down_h = self._insert(self.down_h, flr, False)
            if self.d == IDLE: self.d = DOWN
            
        # always update tf
        self.update_tf()
        
        if vip: print(f"[{self.name}] VIP added: {flr}")
        else:   print(f"[{self.name}] Req added: {flr}")

    def calc_cost(self, rf):
        if self.state == EMERGENCY: return 99999
        res = abs(self.cf - rf)
        if self.d == IDLE: return res

        # penalty logic
        if self.d == UP:
            if rf >= self.cf: return res
            else: return abs(self.tf - self.cf) + abs(self.tf - rf)

        if self.d == DOWN:
            if rf <= self.cf: return res
            else: return abs(self.tf - self.cf) + abs(self.tf - rf)
        return res

    def emergency_stop(self):
        print(f"\n!!! {self.name} STOP !!!")
        self.state = EMERGENCY
        self.d = IDLE
        self.tf = self.cf 
        # clear lists
        self.up_h = None
        self.down_h = None
        self.v_up = None
        self.v_down = None

    def reset(self):
        if self.state != EMERGENCY: return
        print(f"[{self.name}] Reset done.")
        self.state = "ACTIVE"
        self.d = IDLE
        self.tf = self.cf

    # one step movement
    def move_step(self):
        if self.state == EMERGENCY or self.d == IDLE: return

        dest = None
        
        # Absolute VIP Priority Logic
        
        has_v_up = (self.v_up is not None)
        has_v_down = (self.v_down is not None)
        
        if self.d == UP:
            if has_v_up:
                dest = self.v_up.floor
            elif has_v_down:
                # switch for VIP!
                print(f"[{self.name}] Switch DOWN (VIP)")
                self.d = DOWN
                self.update_tf()
                return 
            elif self.up_h:
                dest = self.up_h.floor
            elif self.down_h:
                self.d = DOWN
                self.update_tf()
                return
            else:
                self.d = IDLE
                return

        elif self.d == DOWN:
            if has_v_down:
                dest = self.v_down.floor
            elif has_v_up:
                # switch for VIP!
                print(f"[{self.name}] Switch UP (VIP)")
                self.d = UP
                self.update_tf()
                return
            elif self.down_h:
                dest = self.down_h.floor
            elif self.up_h:
                self.d = UP
                self.update_tf()
                return
            else:
                self.d = IDLE
                return

        # physics move
        if dest is not None:
            if self.cf < dest:
                self.cf += 1
                print(f"[{self.name}] ^ {self.cf}")
            elif self.cf > dest:
                self.cf -= 1
                print(f"[{self.name}] v {self.cf}")
            
            # arrived?
            if self.cf == dest:
                print(f"[{self.name}] * DING * {self.cf}")
                self.hist.push(self.cf)
                
                # remove node
                if self.d == UP:
                    if self.v_up and self.v_up.floor == self.cf:
                        self.v_up = self.v_up.next
                    elif self.up_h and self.up_h.floor == self.cf:
                        self.up_h = self.up_h.next
                else:
                    if self.v_down and self.v_down.floor == self.cf:
                        self.v_down = self.v_down.next
                    elif self.down_h and self.down_h.floor == self.cf:
                        self.down_h = self.down_h.next
                
                self.update_tf() 

class HotelSystem:
    def __init__(self):
        self.elvs = [Elevator("A"), Elevator("B")]

    def req(self, f, vip=False):
        if f < MIN_F or f > MAX_F: return

        c1 = self.elvs[0].calc_cost(f)
        c2 = self.elvs[1].calc_cost(f)

        if c1 <= c2:
            print(f"Req {f} -> A ({c1})")
            self.elvs[0].add_req(f, vip)
        else:
            print(f"Req {f} -> B ({c2})")
            self.elvs[1].add_req(f, vip)

    def run(self):
        self.elvs[0].move_step()
        self.elvs[1].move_step()

if __name__ == "__main__":
    sys = HotelSystem()
    sys.elvs[0].cf = 0
    sys.elvs[1].cf = 0
    
    # test absolute priority
    sys.req(5) 
    sys.req(-2, vip=True) # should force switch
    
    print("\n--- SIMULATION ---")
    for i in range(5):
        print(f"\nTime {i}:")
        sys.run()
    
    print("History of A:",sys.elvs[0].hist.show())
    print("History of B:",sys.elvs[1].hist.show())