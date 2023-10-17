from psana import DataSource
from psmon import publish
from psmon.plots import Image,XYPlot
from psana.psexp.zmq_utils import zmq_send
from kafka import KafkaProducer
import json
import os, sys, time
from mpi4py import MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()
 

os.environ['PS_SRV_NODES']='1'
os.environ['PS_SMD_N_EVENTS']='1'


# passing exp and runnum
exp=sys.argv[1]
runnum=int(sys.argv[2])

# fake-server is a small standalone zmq python script                           
fake_dbase_server=sys.argv[3]


mount_dir = '/sdf/data/lcls/drpsrcf/ffb'
#mount_dir = '/cds/data/drpsrcf'
xtc_dir = os.path.join(mount_dir, exp[:3], exp, 'xtc')
ds = DataSource(exp=exp,run=runnum,dir=xtc_dir,intg_det='andor_vls',batch_size=1)


if ds.is_srv(): # hack for now to eliminate use of publish.local below
    publish.init()
    print(f'waiting for psplot connection...')
    time.sleep(3)
    # TODO: hide zmq_send in DataSource
    producer = KafkaProducer(bootstrap_servers=[fake_dbase_server], value_serializer=lambda m:json.JSONEncoder().encode(m).encode('utf-8'))
    info = {'node': MPI.Get_processor_name(),
            'exp': exp,
            'runnum': runnum,
            'port':publish.port,
            'slurm_job_id':os.environ.get('SLURM_JOB_ID', os.getpid())}
    producer.send("monatest", info)
    #zmq_send(fake_dbase_server=fake_dbase_server, 
    #        node=MPI.Get_processor_name(), 
    #        exp=exp, 
    #        runnum=runnum, 
    #        port=publish.port,
    #        slurm_job_id=os.environ.get('SLURM_JOB_ID', os.getpid()))
 
# we will remove this for batch processing and use "psplot" instead
# publish.local = True


def my_smalldata(data_dict):
    if 'unaligned_andor_norm' in data_dict:
        andor_norm = data_dict['unaligned_andor_norm'][0]
        myplot = XYPlot(0,"Andor (normalized)",range(len(andor_norm)),andor_norm)
        publish.send('ANDOR',myplot)
 
for myrun in ds.runs():
    andor = myrun.Detector('andor_vls')
    timing = myrun.Detector('timing')
    smd = ds.smalldata(filename='mysmallh5.h5',batch_size=1000, callbacks=[my_smalldata])
    norm = 0
    ndrop_inhibit = 0
    for nstep,step in enumerate(myrun.steps()):
        print('step:',nstep)
        for nevt,evt in enumerate(step.events()):
            andor_img = andor.raw.value(evt)
            # also need to check for events missing due to damage
            # (or compare against expected number of events)
            ndrop_inhibit += timing.raw.inhibitCounts(evt)
            smd.event(evt, mydata=nevt) # high rate data saved to h5
            # need to check Matt's new timing-system data on every
            # event to make sure we haven't missed normalization
            # data due to deadtime
            norm+=nevt # fake normalization
            if andor_img is not None:
                print('andor data on evt:',nevt,'ndrop_inhibit:',ndrop_inhibit)
                # check that the high-read readout group (2) didn't
                # miss any events due to deadtime
                if ndrop_inhibit[2]!=0: print('*** data lost due to deadtime')
                # need to prefix the name with "unaligned_" so
                # the low-rate andor dataset doesn't get padded
                # to align with the high rate datasets
                smd.event(evt, mydata=nevt,
                          unaligned_andor_norm=(andor_img/norm))
                norm=0
                ndrop_inhibit=0
