#include "allocator_border_descriptors.h"

allocator_border_descriptors::allocator_border_descriptors(
    size_t memory_size,
    allocator* outer_allocator,
    logger* log,
    allocator_fit_allocation::allocation_mode allocation_mode)
{
    auto got_typename = get_typename();

    if (log != nullptr)
    {
        log->trace(got_typename + " allocator instance construction started")
            ->debug("requested memory size: " + std::to_string(memory_size) + " bytes");
    }

    auto const minimal_trusted_memory_size = get_available_block_service_block_size();

    if (memory_size < minimal_trusted_memory_size)
    {
        auto error_message = "trusted memory size should be GT " + std::to_string(minimal_trusted_memory_size) + " bytes";

        if (log != nullptr)
        {
            log->error(error_message);
        }

        throw allocator::memory_exception(error_message);
    }

    auto const allocator_service_block_size = get_allocator_service_block_size();

    _trusted_memory = outer_allocator == nullptr
        ? ::operator new(memory_size + allocator_service_block_size)
        : outer_allocator->allocate(memory_size + allocator_service_block_size);

    auto* const memory_size_space = reinterpret_cast<size_t*>(_trusted_memory);
    *memory_size_space = memory_size;

    auto* const outer_allocator_pointer_space = reinterpret_cast<allocator**>(memory_size_space + 1);
    *outer_allocator_pointer_space = outer_allocator;

    auto* const logger_pointer_space = reinterpret_cast<logger**>(outer_allocator_pointer_space + 1);
    *logger_pointer_space = log;

    auto* const allocation_mode_space = reinterpret_cast<allocator_fit_allocation::allocation_mode*>(logger_pointer_space + 1);
    *allocation_mode_space = allocation_mode;

    auto* const first_available_block_pointer_space = reinterpret_cast<void**>(allocation_mode_space + 1);
    *first_available_block_pointer_space = reinterpret_cast<void*>(first_available_block_pointer_space + 1);

    auto* const first_available_block_size_space = reinterpret_cast<size_t*>(*first_available_block_pointer_space);
    *first_available_block_size_space = memory_size;

    auto* const first_available_block_next_block_address_space = reinterpret_cast<void**>(first_available_block_size_space + 1);
    *first_available_block_next_block_address_space = nullptr;

    this->trace_with_guard(got_typename + " allocator instance construction finished");
}

allocator_border_descriptors::~allocator_border_descriptors() noexcept
{
    auto got_typename = get_typename();
    this->trace_with_guard(got_typename + " allocator instance destruction started");

    auto const* const logger = get_logger();

    deallocate_with_guard(_trusted_memory);

    if (logger != nullptr)
    {
        logger->trace(got_typename + " allocator instance destruction finished");
    }
}

size_t allocator_border_descriptors::get_trusted_memory_size() const noexcept
{
    return *reinterpret_cast<size_t*>(_trusted_memory);
}

allocator_fit_allocation::allocation_mode allocator_border_descriptors::get_allocation_mode() const noexcept
{
    return *reinterpret_cast<allocator_fit_allocation::allocation_mode*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(size_t) + sizeof(allocator*) + sizeof(logger*));
}

size_t allocator_border_descriptors::get_allocator_service_block_size() const noexcept
{
    auto const memory_size_size = sizeof(size_t);
    auto const outer_allocator_pointer_size = sizeof(allocator*);
    auto const logger_pointer_size = sizeof(logger*);
    auto const allocation_mode_size = sizeof(allocator_fit_allocation::allocation_mode);
    auto const first_available_block_pointer_size = sizeof(void*);

    return memory_size_size + outer_allocator_pointer_size + logger_pointer_size + allocation_mode_size + first_available_block_pointer_size;
}

size_t allocator_border_descriptors::get_available_block_service_block_size() const noexcept
{
    auto const current_block_size = sizeof(size_t);
    auto const next_available_block_pointer_size = sizeof(void*);

    return current_block_size + next_available_block_pointer_size;
}

size_t allocator_border_descriptors::get_occupied_block_service_block_size() const noexcept
{
    auto const current_block_size = sizeof(size_t);

    return current_block_size;
}

void** allocator_border_descriptors::get_first_available_block_address_address() const noexcept
{
    return reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(size_t) + sizeof(allocator*) + sizeof(logger*) + sizeof(allocator_fit_allocation::allocation_mode));
}

void* allocator_border_descriptors::get_first_available_block_address() const noexcept
{
    return *get_first_available_block_address_address();
}

size_t allocator_border_descriptors::get_available_block_size(void const* current_block_address) const
{
    return *reinterpret_cast<size_t const*>(current_block_address);
}

void* allocator_border_descriptors::get_available_block_next_available_block_address(void const* current_block_address) const
{
    return *reinterpret_cast<void* const*>(reinterpret_cast<size_t const*>(current_block_address) + 1);
}

size_t allocator_border_descriptors::get_occupied_block_size(void const* current_block_address) const
{
    return *reinterpret_cast<size_t const*>(current_block_address);
}

void allocator_border_descriptors::dump_trusted_memory_blocks_state() const
{
    if (get_logger() == nullptr)
    {
        return;
    }

    std::string to_dump("|");
    auto memory_size = get_trusted_memory_size();
    auto current_available_block = get_first_available_block_address();
    unsigned char* first_block = reinterpret_cast<unsigned char*>(_trusted_memory) + get_allocator_service_block_size();
    unsigned char* current_block = first_block;

    while (current_block - first_block < memory_size)
    {
        size_t current_block_size;
        if (current_block == current_available_block)
        {
            current_block_size = get_available_block_size(current_block);
            to_dump += "avl ";
            current_available_block = get_available_block_next_available_block_address(current_available_block);
        }
        else
        {
            current_block_size = get_occupied_block_size(current_block);
            to_dump += "occ ";
        }

        to_dump += std::to_string(current_block_size) + "|";
        current_block += current_block_size;
    }

    this->debug_with_guard("Memory state: " + to_dump);
}

void* allocator_border_descriptors::allocate(size_t requested_block_size)
{
    auto const got_typename = get_typename();
    this->trace_with_guard("Method `void *" + got_typename + "::allocate(size_t requested_block_size)` execution started")
        ->debug_with_guard("Requested " + std::to_string(requested_block_size) + " bytes of memory");

    auto requested_block_size_overridden = requested_block_size;
    if (requested_block_size_overridden < sizeof(void*))
    {
        requested_block_size_overridden = sizeof(void*);
    }

    void* previous_block = nullptr, * current_block = get_first_available_block_address();
    void* target_block = nullptr, * previous_to_target_block = nullptr, * next_to_target_block = nullptr;
    auto const available_block_service_block_size = get_available_block_service_block_size();
    auto const occupied_block_service_block_size = get_occupied_block_service_block_size();
    auto const allocation_mode = get_allocation_mode();

    while (current_block != nullptr)
    {
        auto const current_block_size = get_available_block_size(current_block);
        auto const next_block = get_available_block_next_available_block_address(current_block);

        if (current_block_size >= requested_block_size_overridden + occupied_block_service_block_size)
        {
            if (allocation_mode == allocator_fit_allocation::allocation_mode::first_fit ||
                allocation_mode == allocator_fit_allocation::allocation_mode::the_best_fit && (target_block == nullptr || current_block_size < get_available_block_size(target_block)) ||
                allocation_mode == allocator_fit_allocation::allocation_mode::the_worst_fit && (target_block == nullptr || current_block_size > get_available_block_size(target_block)))
            {
                previous_to_target_block = previous_block;
                target_block = current_block;
                next_to_target_block = next_block;
            }

            if (allocation_mode == allocator_fit_allocation::allocation_mode::first_fit)
            {
                break;
            }
        }

        previous_block = current_block;
        current_block = next_block;
    }

    if (target_block == nullptr)
    {
        auto const warning_message = "no memory available to allocate";

        this->warning_with_guard(warning_message)
            ->trace_with_guard("Method `void *" + got_typename + "::allocate(size_t requested_block_size)` execution finished");

        throw memory_exception(warning_message);
    }

    auto const target_block_size = get_available_block_size(target_block);

    if (target_block_size - requested_block_size_overridden - occupied_block_service_block_size < available_block_service_block_size)
    {
        requested_block_size_overridden = target_block_size - occupied_block_service_block_size;
    }

    if (requested_block_size_overridden != requested_block_size)
    {
        this->trace_with_guard("Requested " + std::to_string(requested_block_size) + " bytes, but reserved " + std::to_string(requested_block_size_overridden) + " bytes in according to correct work of allocator");

        requested_block_size = requested_block_size_overridden;
    }

    void* updated_next_block_to_previous_block;

    if (requested_block_size == target_block_size - occupied_block_service_block_size)
    {
        updated_next_block_to_previous_block = next_to_target_block;
    }
    else
    {
        updated_next_block_to_previous_block = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(target_block) + occupied_block_service_block_size + requested_block_size);

        auto* const target_block_leftover_size = reinterpret_cast<size_t*>(updated_next_block_to_previous_block);
        *target_block_leftover_size = target_block_size - occupied_block_service_block_size - requested_block_size;

        auto* const target_block_leftover_next_block_address = reinterpret_cast<void**>(target_block_leftover_size + 1);
        *target_block_leftover_next_block_address = next_to_target_block;
    }

    previous_to_target_block == nullptr
        ? *get_first_available_block_address_address() = updated_next_block_to_previous_block
        : *reinterpret_cast<void**>(reinterpret_cast<size_t*>(previous_to_target_block) + 1) = updated_next_block_to_previous_block;

    auto* target_block_size_address = reinterpret_cast<size_t*>(target_block);
    *target_block_size_address = requested_block_size + sizeof(size_t);

    auto* const allocated_block = reinterpret_cast<void*>(target_block_size_address + 1);

    this->trace_with_guard("Allocated block placed at " + address_to_hex(allocated_block))
        ->trace_with_guard("Method `void *" + got_typename + "::allocate(size_t requested_block_size)` execution finished");

    this->debug_with_guard("After `allocate` for " + std::to_string(requested_block_size) + " bytes (addr == " +
        address_to_hex(target_block_size_address) + "):");
    dump_trusted_memory_blocks_state();
    return allocated_block;
}

void allocator_border_descriptors::deallocate(void* block_to_deallocate_address)
{
    auto const got_typename = get_typename();
    this->trace_with_guard(got_typename + "::deallocate(void *block_to_deallocate_address) execution started");

    //указатель на дескриптор размера блока памяти, расположенного перед адресом, который будет освобожден.
    auto* before_the_block_to_deallocate_size = reinterpret_cast<size_t*>(block_to_deallocate_address) - 1;

    //размер
    size_t block_size = *before_the_block_to_deallocate_size;

    //переводим вв char*, так как такой указатель занимает 1 байт. Это позволяет точно управлять смещениями в памяти
    auto* previous_block_descriptor = reinterpret_cast<size_t*>(reinterpret_cast<char*>(block_to_deallocate_address) - sizeof(size_t)); //указатель на предыдущий блок
    auto* next_block_descriptor = reinterpret_cast<size_t*>(reinterpret_cast<char*>(block_to_deallocate_address) + block_size);//указатель на следующий блок

    //проверка на свободен/занят
    bool previous_block_free = (*previous_block_descriptor & 1) == 0;
    bool next_block_free = (*next_block_descriptor & 1) == 0;

    //указатель на следующий блок
    *before_the_block_to_deallocate_size = block_size;

    if (previous_block_free && next_block_free) {
        //объединение если левый и правый блок свободен
        size_t prev_block_size = *(previous_block_descriptor - 1);
        size_t next_block_size = *next_block_descriptor;

        //объединение
        *previous_block_descriptor = prev_block_size + block_size + next_block_size + 2 * sizeof(size_t);

        auto* new_next_block_descriptor = reinterpret_cast<size_t*>(reinterpret_cast<char*>(block_to_deallocate_address) + prev_block_size);
        *new_next_block_descriptor = *previous_block_descriptor;
    }

    else if (previous_block_free) {
        //объединение если только предыдущий свободен блок
        size_t prev_block_size = *(previous_block_descriptor - 1);

        //объединение
        *previous_block_descriptor = prev_block_size + block_size + sizeof(size_t);

        auto* new_next_block_descriptor = reinterpret_cast<size_t*>(reinterpret_cast<char*>(block_to_deallocate_address) + prev_block_size);
        *new_next_block_descriptor = *previous_block_descriptor;
    }
    else if (next_block_free) {
        //объединение если свободен только следующий блок
        size_t next_block_size = *next_block_descriptor;

        *before_the_block_to_deallocate_size = block_size + next_block_size + sizeof(size_t);
    }

    this->debug_with_guard("After `deallocate` (addr == " + address_to_hex(block_to_deallocate_address) + "):");
    dump_trusted_memory_blocks_state();
    this->trace_with_guard(got_typename + "::deallocate method execution finished");
}

void* allocator_border_descriptors::reallocate(void* block_to_reallocate_address, size_t new_block_size)
{
    auto* new_block = allocate(new_block_size);
    auto occupied_block_service_block_size = get_occupied_block_service_block_size();
    auto data_to_move_size = std::min(get_occupied_block_size(reinterpret_cast<unsigned char const*>(new_block) - occupied_block_service_block_size), get_occupied_block_size(reinterpret_cast<unsigned char const*>(block_to_reallocate_address) - occupied_block_service_block_size)) - occupied_block_service_block_size;
    memcpy(new_block, block_to_reallocate_address, data_to_move_size);
    deallocate(block_to_reallocate_address);
    return new_block;
}

bool allocator_border_descriptors::reallocate(void** block_to_reallocate_address_address, size_t new_block_size)
{
    try {
        *block_to_reallocate_address_address = reallocate(*block_to_reallocate_address_address, new_block_size);
        return true;
    }
    catch (std::exception const& ex)
    {
        this->warning_with_guard(ex.what());
        return false;
    }
}

void allocator_border_descriptors::setup_allocation_mode(allocator_fit_allocation::allocation_mode mode)
{
    *reinterpret_cast<allocator_fit_allocation::allocation_mode*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(size_t) + sizeof(allocator*) + sizeof(logger*)) = mode;
}

logger* allocator_border_descriptors::get_logger() const noexcept
{
    return *reinterpret_cast<logger**>(reinterpret_cast<allocator**>(reinterpret_cast<size_t*>(_trusted_memory) + 1) + 1);
}

std::string allocator_border_descriptors::get_typename() const noexcept
{
    return "allocator_border_descriptors";
}

allocator* allocator_border_descriptors::get_allocator() const noexcept
{
    return *reinterpret_cast<allocator**>(reinterpret_cast<size_t*>(_trusted_memory) + 1);
}

