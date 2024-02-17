import {type Writable, writable} from 'svelte/store';

export type DeviceData = {
    client_id: string;
    device_id: string;
    device_nodes: string[];
    type: string;
};

export type JsonError = {
    error: string;
}

/**
 * A list of all the devices available on the server
 */
export const available_devices = writable([] as DeviceData[]);

/**
 * The currently selected device to be used in the UI
 */
export const selected_device: Writable<DeviceData | null> = writable(null);

/**
 * Used to show errors in the UI when connecting to the API
 */
export const fetch_error: Writable<Error[]> = writable([]);

export async function request(endpoint: string, method = "GET", body = {}): Promise<any> {
    let opts: RequestInit = {
        method: method
    };
    if (method != "GET") {
        opts = {
            ...opts,
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify(body)
        }
    }
    const response = await fetch("/api/v1.0" + endpoint, opts);
    if (response.ok) {
        return response.json();
    } else {
        const error_msg = (await response.json().catch(_error => {
            return {error: response.statusText};
        }) as JsonError).error;
        const error = new Error(error_msg);
        console.error(error_msg);
        fetch_error.update(errors => errors.concat(error));
        return Promise.reject(error);
    }
}
