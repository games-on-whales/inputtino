<script lang="ts">
    import {type DeviceData, type JsonError, available_devices, selected_device, request} from "./stores";
    import {Dropdown, DropdownItem, ToolbarButton} from "flowbite-svelte";
    import {ChevronRightSolid, CirclePlusSolid} from "flowbite-svelte-icons";

    async function add_device(type: string, additional_data: any = {}) {
        const device: DeviceData = await request('/devices/add', "POST", Object.assign(additional_data, {
            type: type
        }));
        available_devices.update(devices => [...devices, device]);
        selected_device.set(device);
    }
</script>

<ToolbarButton>
    <CirclePlusSolid class="text-purple-700 dark:text-purple-400"></CirclePlusSolid>
    <Dropdown>
        <DropdownItem on:click={() => add_device("MOUSE")}>Mouse</DropdownItem>
        <DropdownItem on:click={() => add_device("KEYBOARD")}>Keyboard</DropdownItem>
        <DropdownItem class="flex items-center justify-between">
            Joypad
            <ChevronRightSolid class="w-3 h-3 ms-2 text-primary-700 dark:text-white"/>
        </DropdownItem>
        <Dropdown placement="right-start">
            <DropdownItem on:click={() => add_device("JOYPAD", {"joypad_type": "xbox"})}>Xbox</DropdownItem>
            <DropdownItem on:click={() => add_device("JOYPAD", {"joypad_type": "ps"})}>PS</DropdownItem>
            <DropdownItem on:click={() => add_device("JOYPAD", {"joypad_type": "nintendo"})}>Nintendo</DropdownItem>
        </Dropdown>
    </Dropdown>
</ToolbarButton>