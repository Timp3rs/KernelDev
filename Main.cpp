#include <ntddk.h>
#include <string.h>

#define MYMethodNeither CTL_CODE(0x8001,0x801,METHOD_NEITHER,FILE_ANY_ACCESS)

struct Data
{
	char name[200];
	int age;
};



DRIVER_UNLOAD DrvUnload;
DRIVER_DISPATCH DevCtrl;
DRIVER_DISPATCH CreateClose;

#define Driv_Pref "IRPTest : "

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObj, PUNICODE_STRING RegPath) {
	NTSTATUS status;
	UNREFERENCED_PARAMETER(RegPath);
	bool symcreated = false;

	KdPrint(("DriverEntry Called...!"));
	UNICODE_STRING DevName = RTL_CONSTANT_STRING(L"\\Device\\TestIRP");
	UNICODE_STRING Symlink = RTL_CONSTANT_STRING(L"\\??\\TestIrp");


	PDEVICE_OBJECT Devobj = nullptr;
	do
	{
		status = IoCreateDevice(DriverObj, 0, &DevName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, TRUE, &Devobj);

		if (!NT_SUCCESS(status))
		{
			KdPrint((Driv_Pref "Create Device Failed. %08x", status));
			break;
		}
		Devobj->Flags |= DO_BUFFERED_IO;


		status = IoCreateSymbolicLink(&Symlink, &DevName);
		if (!NT_SUCCESS(status))
		{
			KdPrint((Driv_Pref "Failed to Create Symbolic Link. %08x", status));
			break;
		}
		symcreated = true;
		

	} while (false);
	
	if (!NT_SUCCESS(status))
	{
		if (symcreated)
		{
			KdPrint(("111"));
			IoDeleteSymbolicLink(&Symlink);
		}

		if (Devobj)
		{
			KdPrint(("112221"));
			IoDeleteDevice(Devobj);
		}
	}

	DriverObj->DriverUnload = DrvUnload;
	DriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DevCtrl;
	DriverObj->MajorFunction[IRP_MJ_CREATE] = DriverObj->MajorFunction[IRP_MJ_CLOSE] = CreateClose;

	return status;

}

NTSTATUS DevCtrl(PDEVICE_OBJECT, PIRP irp) {

	KdPrint(("devctrl Is Called"));

	PIO_STACK_LOCATION stloc;
	NTSTATUS status = STATUS_SUCCESS;

	stloc = IoGetCurrentIrpStackLocation(irp);

	switch (stloc->Parameters.DeviceIoControl.IoControlCode)
	{
	case MYMethodNeither:
	{
		//check size of data then process

		auto data = (Data*)stloc->Parameters.DeviceIoControl.Type3InputBuffer;

		if (data == nullptr)
		{
			KdPrint((Driv_Pref "ERROR To Fetch Data in Neither Method!"));
			break;
		}
		KdPrint((Driv_Pref "Fetched Data => %s", data->name));
		KdPrint((Driv_Pref "Fetched Data => %d", data->age));
		
		break;
	}
	default:
	{
		KdPrint(("Invalid IOControlCode Recieved...!"));
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS CreateClose(PDEVICE_OBJECT devobj, PIRP irp) {
	UNREFERENCED_PARAMETER(devobj);
	PIO_STACK_LOCATION stloc = IoGetCurrentIrpStackLocation(irp);
	switch (stloc->MajorFunction)
	{
	case IRP_MJ_CREATE:
		KdPrint(("Create Is Called"));
		break;
	case IRP_MJ_CLOSE:
		KdPrint(("Close Is Called"));
		break;
	default:
		break;
	}
	//check request is create or close...!
	irp->IoStatus.Information = 0;
	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp,IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

void DrvUnload(PDRIVER_OBJECT DriverObj) {
	KdPrint(("Unload Is Called"));
	UNREFERENCED_PARAMETER(DriverObj);
	UNICODE_STRING Symlink = RTL_CONSTANT_STRING(L"\\??\\TestIrp");

	IoDeleteSymbolicLink(&Symlink);
	IoDeleteDevice(DriverObj->DeviceObject);
}
